// Superimer les headers inutils
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

// Verification des appels de primitives systemes
#define CHECK(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
    } while (0)

// Fonction raler amelioree
noreturn void raler(int syserr, const char* fmt, ...);

void computeRandNumList(uint8_t n, uint8_t nMax, uint8_t *numbersList);

void parseStrToNumList(int nChar, char* charList[], uint8_t *numbersList);

int main(int argc, char *argv[]){
    int c;
    bool errFlag = false;
    extern char *optarg;
    extern int optind;

    char *nCh, nMaxCh;          // n, et nmax en string
    bool r = false;             // Indique si l'option r a ete donnee ou pas
    bool m = false;             // Indique si l'option m
    uint8_t n, nMax;
    char *inputList;            // Liste des nombres au format texte
    uint8_t *numbersList;       // La liste des nombres a trier

    while ((c = getopt(argc, argv, "r:m:") != -1)){
        switch (c){
        case 'r':
            r = true;
            // nCh = optarg;
            n = atoi(optarg);
            if (n <= 0 || n > 255)
                raler (0, "Debordement de limite ou conversion impossible");
            break;
        case 'm':
            m = true;
            // nMaxCh = optarg;
            nMax = atoi(optarg);
            if (nMax <= 0 || nMax > 255)
                raler (0, "Debordement de limite ou conversion impossible");
            break;
        default:
            raler(0, "Argument inconnu");
            break;
        }
    } 
    
    if (r == true){
        numbersList = malloc(n);
        if (numbersList == NULL)
            raler(1, "Erreur d'allocation");
        if (m = false)
            computeRandNumList(n, n, numbersList);
        else    // m == true
            computeRandNumList(n, nMax, numbersList);
    } else if (r == false && m == true)
        raler (0, "Fournissez le n");
    else{ // r == false && m == false   // Les nombres ont ete fournis
        n = argc - 1; 
        numbersList = malloc(n);
        if (numbersList == NULL)
            raler(1, "Erreur d'allocation");
        parseStrToNumList(n, argv, numbersList);
    }

    return 0;

}

void computeRandNumList(uint8_t n, uint8_t nMax, uint8_t *numbersList){
    for (uint8_t i = 0; i < n; i++)
        numbersList[i] = rand()%nMax;
}

void parseStrToNumList(int nChar, char* charList[], uint8_t *numbersList){
    int temp;
    for (uint8_t i = 0; i < nChar; i++){
        temp = atoi(charList[i+1]);
        if (temp <= 0 || temp > 255)
            raler(0, "Debordement de limite ou conversion impossible");
        numbersList[i] = temp;
    }
}

noreturn void raler(int syserr, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr) perror("");
    exit(1);
}