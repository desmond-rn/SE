#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <limits.h>

#define CHEMIN_MAX 512

// Verifie les appels aux primitives systemes
#define CHK_PS(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
    } while (0)

// Verifie les appels à malloc et realloc
#define CHK_MA(v)                     \
    do {                             \
        if ((v) == NULL) raler(1, #v); \
    } while (0)

// Verifie les tailles des chemins
#define CHK_SN(v)                     \
    do {                             \
        if (((v) > CHEMIN_MAX) || ((v) <= 0)) raler(0, "%s\n%s", #v ,"Oops! Chemin trop long, ou autre erreur..."); \
    } while (0)

// Rale
noreturn void raler(int syserr, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr) perror("");
    exit(1);
}

// Execute la cmd sur chaque chemin trouve
void execChild(char *path, int argC, char *argV[]){
    switch (fork()){
    case -1:
        raler(1, "Zut! Erreur de fork");
        break;
    case 0:
        argV[argC-2] = path;
        execvp(argV[0], argV);
        raler(1, "Erreur d'execvp");
        break;
    default:
        break;
    }
}

// Fonction pour attendre les nChilds
void waitChildren(int nChilds){
    int raison;
    for (int i = 0; i < nChilds; i++){
        CHK_PS(wait(&raison));
        if (!(WIFEXITED(raison) && WEXITSTATUS(raison) == 0))
            raler(0, "Ce fils ne s'est pas terminé comme prévu");
    }
}

// Fonction pour creer des chemin par lecture en bloc et puis lance la cmd
void makePaths(const char *buffer, size_t bufferSize, int argC, char *argV[]){
    // Creation du chemin et exec
    char *path;     // On ramène tout a des char...
    CHK_MA(path = malloc(CHEMIN_MAX+1));
    int pathSize = 0;
    int nChilds = 0;
    for(size_t i=0; i<bufferSize; i++){
        if (buffer[i] != '\n')
            pathSize++;
        else{
            CHK_SN(pathSize);
            CHK_MA(memmove(&path[0], &buffer[i-pathSize], pathSize));
            path[pathSize] = '\0';
            pathSize = 0;
            execChild(path, argC, argV);
            nChilds ++;
        } 
    }
    free(path);

    // Attente des fils
    waitChildren(nChilds);
}

// Programme principal en vue de la pagination
int myMainProgram(int argc, char *argv[]){
    // Verification du nombre d'arguments
    if (argc < 3)
        raler(0, "Nombre d'arguments insuffisant");

    // Validité du repertoire donné 
    int dirLength = strlen(argv[1]);
    CHK_SN(dirLength);

    // Constitution des arguements
    char *args[argc];
    for (int i = 0; i < argc-2; i++)
        args[i] = argv[2+i];
    args[argc-2] = NULL;      // Valeur à modifier plus tard!!!!
    args[argc-1] = NULL; 

    // tube pour l'execution du find
    int tube[2];
    CHK_PS(pipe(tube));
    switch (fork()){
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
            unsigned char *buffer;
            CHK_MA(buffer = malloc(PIPE_BUF));
            size_t bufferSize;
            while ((bufferSize = read(tube[0], buffer, PIPE_BUF)) > 0)
                makePaths(buffer, bufferSize, argc, args);
            // On s'assure que le le "find" s'est bien arreté
            waitChildren(1);
            CHK_PS(close(tube[0]));
            break;
    }

    return 0;
}

// Main
int main(int argc, char *argv[]){
    // Tube pour la pagination
    int page[2];
    CHK_PS(pipe(page));
    switch (fork()){
    case -1:
        raler(1, "fork failed");
        break;
    case 0:;
        CHK_PS(close(page[0]));
        CHK_PS(dup2(page[1], 1));
        CHK_PS(close(page[1]));
        int retVal = myMainProgram(argc, argv);
        if (retVal != 0)
            raler(0, "Programme executé avec des erreurs");
        exit(0);
        break;
    default:
        CHK_PS(close(page[1]));
        CHK_PS(dup2(page[0], 0));
        CHK_PS(close(page[0]));
        // Attente du programme principal
        waitChildren(1);    // Inefficace mais bon ...
        execlp("more", "more", NULL);   // Le parent doit executer le "more"!
        raler(1, "execlp failed!");
    }
}