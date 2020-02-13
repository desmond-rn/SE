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
#include <sys/wait.h>
#include <getopt.h>

// Verification des appels de primitives systemes
#define CHECK(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
    } while (0)

// Fonction raler amelioree
noreturn void raler(int syserr, const char* fmt, ...);

void computeRandNumList(uint8_t n, uint8_t nMax, uint8_t *numbersList);

void parseStrToNumList(int nChar, char* charList[], uint8_t *numbersList);

bool parseCmdLineArg(int argc, char *argv[], char *format, int64_t *n, int64_t *nMax);

int main(int argc, char *argv[]){
    int c;
    bool errFlag = false;
    extern char *optarg;
    extern int optind;

    char *nCh, nMaxCh;          // n, et nmax en string
    bool rFlag = false;             // Indique si l'option r a ete donnee ou pas
    bool mFlag = false;             // Indique si l'option m
    int64_t n, nMax;            // 64 bits pour detecter les grosses valeurs
    char *inputList;            // Liste des nombres au format texte
    uint8_t *numbersList;       // La liste des nombres a trier

//-----------------------------

    while ((c = getopt(argc, argv, "r:m:")) != -1){
        switch (c){
            case 'r':
                rFlag = true;
                n = atoi(optarg);
                break;
            case 'm':
                mFlag = true;
                nMax = atoi(optarg);
                break;
            default:
                raler(0, "Argument inconnu");
                break;
        }
        // if (mFlag){

        // }
    } 


//-----------------------------

    // bool parse = parseCmdLineArg(argc, argv, "r:m:", &n, &nMax);

    if (rFlag == true){
        // n = atoi(nCh);
        if (n <= 0 || n > 255)
            raler (0, "Debordement de limite ou conversion impossible");
        numbersList = malloc(n);
        if (numbersList == NULL)
            raler(1, "Erreur d'allocation");
        if (mFlag == false){
            computeRandNumList(n, n, numbersList);
        }
        else{    // m == true
            // nMax = atoi(nMaxCh);
            if (nMax <= 0 || nMax > 255)
                raler (0, "Debordement de limite ou conversion impossible");
            computeRandNumList(n, nMax, numbersList);
        }
    } else if (rFlag == false && mFlag == true)
        raler (0, "Fournissez le n");
    else{ // r == false && m == false   // Les nombres ont ete fournis
        n = argc - 1; 
        numbersList = malloc(n);
        if (numbersList == NULL)
            raler(1, "Erreur d'allocation");
        parseStrToNumList(n, argv, numbersList);
    }

    // Juste pour tester
    for (uint8_t i = 0; i < n; i++)
        printf ("%d\t", numbersList[i]);
    printf ("\n");

    pid_t pid; 
    int raison;
    bool isChild = false;
    uint8_t localValue = 0;
    uint64_t sum = 0; 

    uint8_t i = 0;
    while (!isChild && i < n){
        switch(pid = fork()){
            case -1:
                raler (0, "Erreur de fork");
                break;
            case 0:
                isChild = true;
                localValue = numbersList[i];
                break;
            default:        // On est dans le processus pere
                break;
        }
        i++;
    }
   
    if (isChild){
        CHECK(usleep(localValue*100000));
        printf("%d\n", localValue);
        exit(localValue);
    }else{
        i = 0;
        while (i < n){             
            wait(&raison);
            if(WIFEXITED(raison))
                sum += WEXITSTATUS(raison);
            i++;
        }
        printf("Sum = %ld\n", sum);
    }

    return 0;

}

// bool parseCmdLineArg(int argc, char *argv[], char *format, int64_t *n, int64_t *nMax){
//     int c;
//     bool r = false, m = false;
//     // if (strcmp(format, "r"))
//     while ((c = getopt(argc, argv, format)) != -1){
//         switch (c){
//         case 'r':
//             r = true;
//             *n = atoi(optarg);
//             break;
//         case 'm':
//             m = true;
//             *nMax = atoi(optarg);
//             break;
//         default:
//             raler(0, "Argument inconnu");
//             break;
//         }
//     }
//     return (r || r&&m) && (optind == argc); 
// }

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