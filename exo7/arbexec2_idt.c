#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define CHEMIN_MAX 512

// Verifie les appels aux primitives systemes
#define CHK_PS(v)         \
    do {                  \
        if ((v) == -1)    \
            raler(1, #v); \
    } while (0)

// Verifie la validite des pointeurs
#define CHK_MA(v)         \
    do {                  \
        if ((v) == NULL)  \
            raler(1, #v); \
    } while (0)

// Verifie les tailles des chemins
#define CHK_SN(v)                                                \
    do {                                                         \
        if (((v) > CHEMIN_MAX) || ((v) <= 0))                    \
            raler(0, "%s\n%s", #v,                               \
                  "Oops! Chemin trop long, ou autre erreur..."); \
    } while (0)

// Rale
noreturn void raler(int syserr, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr)
        perror("");
    exit(1);
}

// Execute la commande de argV sur le chemin donné
void execChild(char *path, int argC, char *argV[]) {
    switch (fork()) {
        case -1:
            raler(1, "Zut! Erreur de fork");
            break;
        case 0:
            argV[argC - 2] = path;
            execvp(argV[0], argV);
            raler(1, "Erreur d'execvp");
            break;
        default:
            break;
    }
}

// Fonction pour attendre nChilds fils
// Retourne le nombre de fils qui ont echoué
int waitChildren(int nChilds) {
    int raison;
    int nErrors = 0;
    for (int i = 0; i < nChilds; i++) {
        CHK_PS(wait(&raison));
        if (!(WIFEXITED(raison) && WEXITSTATUS(raison) == 0))
            nErrors++;
    }
    return nErrors;
}

// Fonction pour creer des chemin par lecture en bloc et puis lance la cmd
// Retourne le nombre de fils qui ont echoué pendant cet appel
int makePaths(const char *buffer, size_t bufferSize, int argC, char *argV[]) {
    // Chemin
    char *path;
    CHK_MA(path = malloc(CHEMIN_MAX + 1));
    int pathSize = 0;
    int nChilds = 0;

    // Chemin temporaire au cas ou le buffer ne se termine pas par '\n'
    static char tmpPath[CHEMIN_MAX + 1];  // Pour eviter d'avoir a faire free()
    static int tmpPathSize = 0;

    // Remplissage du chemin et exec
    for (size_t i = 0; i < bufferSize; i++) {
        if (buffer[i] != '\n')
            pathSize++;
        else {
            CHK_SN(tmpPathSize + pathSize);
            if (tmpPathSize > 0)
                CHK_MA(memmove(path, tmpPath, tmpPathSize));
            CHK_MA(
                memmove(&path[tmpPathSize], &buffer[i - pathSize], pathSize));
            path[tmpPathSize + pathSize] = '\0';
            execChild(path, argC, argV);
            pathSize = 0;
            tmpPathSize = 0;
            nChilds++;
        }
    }
    free(path);

    // Gestion des portions de chemin a la fin de l'entree standard
    if (buffer[bufferSize - 1] != '\n') {
        CHK_SN(pathSize);
        CHK_MA(memmove(tmpPath, &buffer[bufferSize - pathSize], pathSize));
        tmpPathSize = pathSize;
        tmpPath[pathSize] = '\0';
    }

    // Attente des fils
    int errors = waitChildren(nChilds);

    return errors;
}

// Programme principal en vue de la pagination
// Retourne le nombre total de ses fils qui ont echoué
int myMainProgram(int argc, char *argv[]) {
    // Verification du nombre d'arguments
    if (argc < 3)
        raler(0, "Nombre d'arguments insuffisant");

    // Validité du repertoire donné
    int dirLength = strlen(argv[1]);
    CHK_SN(dirLength);

    // Constitution des arguements
    char *args[argc];
    for (int i = 0; i < argc - 2; i++)
        args[i] = argv[2 + i];
    args[argc - 2] = NULL;  // Valeur à modifier plus tard!!!!
    args[argc - 1] = NULL;

    // tube pour l'execution du find
    int tube[2];
    CHK_PS(pipe(tube));
    switch (fork()) {
        case -1:
            raler(1, "fork failed!");
            break;
        case 0:
            CHK_PS(close(tube[0]));
            CHK_PS(dup2(tube[1], 1));
            CHK_PS(close(tube[1]));
            execlp("find", "find", argv[1], "-type", "f", NULL);
            raler(1, "execlp failed!");
        default:;
            CHK_PS(close(tube[1]));
            // Lecture du tube
            char *buffer;
            CHK_MA(buffer = malloc(PIPE_BUF));
            size_t bufferSize;
            int errors = 0;  // Comptabilise les processus echoués
            while ((bufferSize = read(tube[0], buffer, PIPE_BUF)) > 0)
                errors += makePaths(buffer, bufferSize, argc, args);
            free(buffer);
            // Attente du processus "find"
            errors += waitChildren(1);
            CHK_PS(close(tube[0]));
            if (errors)
                return errors;
            break;
    }

    return 0;
}

// Main
int main(int argc, char *argv[]) {
    // Avec pagination
    int page[2];
    CHK_PS(pipe(page));
    switch (fork()) {
        case -1:
            raler(1, "fork failed");
            break;
        case 0:;
            CHK_PS(close(page[1]));
            CHK_PS(dup2(page[0], 0));
            CHK_PS(close(page[0]));
            execlp("more", "more", NULL);
            raler(1, "execlp failed!");
            break;
        default:;
            CHK_PS(close(page[0]));
            CHK_PS(dup2(page[1], 1));
            CHK_PS(close(page[1]));
            int nFailed = myMainProgram(argc, argv);
            CHK_PS(close(1));
            nFailed += waitChildren(1);  // Important d'attendre, sinon 'zombie'
            if (nFailed)
                raler(0, "-----\n%d processus fils ont echoué\n-----", nFailed);
            exit(0);
            break;
    }
}