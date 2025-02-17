#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#define CHEMIN_MAX 512
#define INCREMENT 64
// Verifie les appels aux primitives systemes
#define CHK_PS(v)                    \
    do {                             \
        if ((v) == -1) raler(1, #v); \
    } while (0)
// Verifie les appels a malloc et realloc
#define CHK_MA(v)                      \
    do {                               \
        if ((v) == NULL) raler(1, #v); \
    } while (0)
// Verifie les appels a snprintf
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
    if (syserr) perror("");
    exit(1);
}
// Tableau extensible contenant une liste de chaines de caracteres
struct tabext {
    size_t rSize, eSize;  // real size and effective size
    char **list;
};
// Initialise le tableau extensible
void initTabext(struct tabext *extArray) {
    extArray->rSize = extArray->eSize = 0;
    CHK_MA(extArray->list = malloc(INCREMENT * sizeof(char *)));
    extArray->eSize = INCREMENT;
}
// Realloue le tableau extensible
void reallocTabext(struct tabext *extArray) {
    CHK_MA(extArray->list = realloc(
               extArray->list, (extArray->eSize + INCREMENT) * sizeof(char *)));
    extArray->eSize += INCREMENT;
}
// Desalloue le tableau extensible
void freeTabext(struct tabext *extArray) {
    for (size_t i = 0; i < extArray->rSize; i++) free(extArray->list[i]);
    free(extArray->list);
    extArray->list = NULL;
    extArray->eSize = extArray->rSize = 0;
}
// Fonction qui parcours l'arborescence de manière recursive
void scanDirectory(bool sFlag, const char *path, int argsC, char *argsV[]) {
    struct tabext files;  // Tableau des fichiers reguliers dans le rep
    initTabext(&files);
    struct tabext dirs;  // Tableau des sous-repertoires
    initTabext(&dirs);
    DIR *dir;
    dir = opendir(path);
    if (dir == NULL) raler(1, "Sorry! problème d'ouverture du rep: %s", path);
    struct dirent *dirEnt;
    struct stat entStats;
    char *entPath;
    CHK_MA(entPath = malloc(CHEMIN_MAX));
    while ((dirEnt = readdir(dir)) != NULL) {
        if (strcmp(dirEnt->d_name, ".") != 0 &&
            strcmp(dirEnt->d_name, "..") != 0) {
            int entPathLength =
                snprintf(entPath, CHEMIN_MAX, "%s/%s", path, dirEnt->d_name);
            CHK_SN(entPathLength);
            CHK_PS(lstat(entPath, &entStats));
            switch (entStats.st_mode & S_IFMT) {
                case S_IFREG:
                    if (files.rSize == files.eSize) reallocTabext(&files);
                    CHK_MA(files.list[files.rSize] = malloc(entPathLength + 1));
                    CHK_SN(snprintf(files.list[files.rSize], CHEMIN_MAX, "%s",
                                    entPath));
                    files.rSize++;
                    break;
                case S_IFDIR:
                    if (dirs.rSize == dirs.eSize) reallocTabext(&dirs);
                    CHK_MA(dirs.list[dirs.rSize] = malloc(entPathLength + 1));
                    CHK_SN(snprintf(dirs.list[dirs.rSize], CHEMIN_MAX, "%s",
                                    entPath));
                    dirs.rSize++;
                    break;
                default:
                    break;  // Inclus le cas S_IFLNK
            }
        }
    }
    CHK_PS(closedir(dir));
    free(entPath);
    // Traitement des fichiers réguliers
    int fd;
    for (size_t i = 0; i < files.rSize; i++) {
        switch (fork()) {
            case -1:
                raler(1, "Zut! Erreur de fork");
                break;
            case 0:
                if (!sFlag)
                    argsV[argsC - 2] = files.list[i];
                else {
                    CHK_PS(fd = open(files.list[i], O_RDONLY));
                    CHK_PS(dup2(fd, 0));
                    CHK_PS(close(fd));
                }
                execvp(argsV[0], argsV);
                raler(1, "Erreur d'exec");
                break;
            default:
                break;
        }
    }
    // Attente des processus fils
    int raison;
    for (size_t i = 0; i < files.rSize; i++) {
        CHK_PS(wait(&raison));
        if ((WIFEXITED(raison) && WEXITSTATUS(raison) != 0))
            exit(2);
        else if (WIFSIGNALED(raison) || WIFSTOPPED(raison))
            raler(0, "L'un des processus ne s'est pas terminé comme prévu");
    }
    freeTabext(&files);
    // Traitement des sous-repertoires
    for (size_t i = 0; i < dirs.rSize; i++)
        scanDirectory(sFlag, dirs.list[i], argsC, argsV);
    freeTabext(&dirs);
}
// Main
int main(int argc, char *argv[]) {
    int c;
    extern int optind;
    extern int opterr;
    opterr = 0;  // Pour afficher mes messages d'erreur
    bool sFlag = false;
    while ((c = getopt(argc, argv, "+s")) != -1) {
        switch (c) {
            case 's':
                sFlag = true;
                break;
            case '?':
                raler(0, "Option inconnue: %s", argv[optind - 1]);
                break;
            default:
                raler(0, "Erreur dans 'getopt'");
                break;
        }
    }
    if (argc - optind < 2) raler(0, "Nombre d'arguments insuffisant");
    // Constitution du repertoire racine
    char *dir;
    CHK_MA(dir = malloc(strlen(argv[optind]) + 1));
    CHK_SN(snprintf(dir, CHEMIN_MAX, "%s", argv[optind]));
    // Constitution des arguements
    char *args[argc - optind - 1 + 2];
    for (int i = 0; i < argc - optind - 1; i++) args[i] = argv[optind + 1 + i];
    args[argc - optind - 1] = NULL;  // Sauf si -s n'est pas donné!!!!
    args[argc - optind] = NULL;
    // Exploration de l'arborescence
    scanDirectory(sFlag, dir, argc - optind + 1, args);
    free(dir);
    return 0;
}