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
    bool errFlag = false;

    bool rFlag = false;             // Indique si l'option r a ete donnee ou pas
    bool mFlag = false;             // Indique si l'option m a ete fournie
    int n, nMax;                // 64 bits pour detecter les grosses valeurs
    int *numbersList;           // La liste des nombres a trier

    if (argc == 1)
        raler(0, "Precisez une syntaxe pour le programme");

    while ((c = getopt(argc, argv, ":rm:")) != -1){ 
        switch (c){
            case 'r':
                rFlag = true;   // On peut fournir l'option -r plusieurs fois
                break;
            case 'm':           // On peut fournir l'option -m plusieurs fois
                mFlag = true;
                nMax = atoi(optarg);
                break;
            case ':':
                raler(0, "Fournissez l'argument de -m");
                break;
            case '?':
                raler(0, "Option inconnue: %s", argv[optind-1]);
                break;
            default:
                errFlag = true;
                break;
        }
        if (errFlag)
            raler(0, "Erreur dans 'getopt'");
    } 

    if (rFlag){ // SCENARIO 1, on calcule aleatoirement les nombres
        if (optind > argc - 1)              // On a parcouru tous les arguments
            raler(0, "Fournissez l'argument de -r");
        else if (optind < argc - 1)         // 'getopt' s'est arrete trop tot
            raler(0, "Vous avez fournis trop d'arguments");
        else{                               // optind == argc - 1
            n = atoi(argv[optind]);
            }
        }


    return 0;
}