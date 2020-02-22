#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <getopt.h>

#define CHEMIN_MAX 512

// Verifie les appels aux primitives systemes
#define CHK(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
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

int main(int argc, char *argv[]){
    int c;
    extern char *optarg;
    extern int optind;

    bool sFlag = false;

    if (argc < 3)
        raler(0, "Arguments insuffisants");

    while ((c = getopt(argc, argv, "+s")) != -1){        // s ou +s
        switch (c){
            case 's':
                sFlag = true;
                break;
            case '?':
                raler(0, "Option inconnue: %s", argv[optind-1]);
                break;
            default:
                raler(0, "Erreur dans 'getopt'");
                break;
        }

    } 

    char *dir = malloc(strlen(argv[optind])+1);
    if (dir == NULL)
        raler(1, "Oops! malloc a echoue");
    if (snprintf(dir, CHEMIN_MAX, "%s", argv[optind]) > CHEMIN_MAX)
        raler(0, "Oops! Chemin trop long");
    printf("Dir path is: %s\n", dir);

    char *cmd = malloc(strlen(argv[optind+1])+1);
    if (cmd == NULL)
        raler(1, "Oops! malloc a echoue");
    snprintf(cmd, CHEMIN_MAX, "%s", argv[optind+1]);   // Demande ?
    printf("Comand is: %s\n", cmd);

    // printf("%d\n", argc-optind-2);
    char **arg = malloc((argc-optind-2)*sizeof(char*));
    if (arg == NULL)
        raler(1, "Oops! malloc a echoue");
    for (int i = 0; i < argc-optind-2; i++){
        arg[i] = malloc(strlen(argv[optind+2+i])+1);
        if (arg[i] == NULL)
            raler(1, "Oops! malloc a echoue");
        snprintf(arg[i], CHEMIN_MAX, "%s", argv[optind+2+i]);
        printf("%s\n", arg[i]);
    }
    

    if (!sFlag){


    }


    printf("\n");
    return 0;
}