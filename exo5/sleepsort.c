#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <getopt.h>

#define RATIO 10        // Rapport variable/temps pris pour l'affchier

// Verifie les appels aux primitives systemes
#define CHECK(v)                     \
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

// Calcule une liste de n nombre pseudo-aleatoirement entre 0 et nMax inclu
void computeRandNumList(uint8_t n, uint8_t nMax, uint8_t *numbersList){
    srand(time(NULL));      // Reinitialiser le noyau
    for (uint8_t i = 0; i < n; i++)
        // numbersList[i] = rand()%(nMax+1);        // Pas assez random
        // On calcule divise essentiellement rand() par RAND_MAX ... (en double)
        numbersList[i] = (int)((double)rand()*(nMax+1) / ((double)RAND_MAX+1));
}

// Converti une liste de chaine de charactere en liste d'entier
void parseStrToNumList(int nStr, char* strList[], uint8_t *numbersList){
    int temp;
    for (uint8_t i = 0; i < nStr; i++){
        temp = atoi(strList[i+1]);
        if (temp <= 0 || temp > 255)
            raler(0, "Debordement de limite ou conversion impossible");
        numbersList[i] = temp;
    }
}

int main(int argc, char *argv[]){
    int c;
    extern char *optarg;
    extern int optind;

    bool rFlag = false;             // Indique si l'option r a ete donnee ou pas
    bool mFlag = false;             // Indique si l'option m a ete fournie
    int64_t n, nMax;                // 64 bits pour detecter les grosses valeurs
    uint8_t *numbersList;           // La liste des nombres a trier

    while ((c = getopt(argc, argv, ":rm:")) != -1){ 
        switch (c){
            case 'r':
                rFlag = true;
                break;
            case 'm':
                mFlag = true;
                nMax = atoi(optarg);
                break;
            case ':':
                raler(0, "Fournissez l'argument de -m");
                break;
            default:
                if (rFlag)
                    raler(0, "Option inconnue");
                break;
        }
    } 
    if (c == -1){
        if (optind == argc-1 && rFlag){
            n = atoi(argv[optind]);
        } else if (optind >= argc && rFlag)          // On a parcouru toute la lsite des arguments 
            raler(0, "Fournissez l'argument de -r");
    }

    if (rFlag){     // SCENARIO 1, on calcule aleatoirement les nombres
        if (n <= 0 || n > 255)
            raler (0, "Debordement de limite pour 'n', ou conversion impossible");
        numbersList = malloc(n);
        if (numbersList == NULL)
            raler(1, "Erreur d'allocation");
        if (!mFlag)
            computeRandNumList(n, n, numbersList);
        else{    // mFlag == true
            if (nMax <= 0 || nMax > 255)
                raler (0, "Debordement de limite pour 'nmax', ou conversion impossible");
            computeRandNumList(n, nMax, numbersList);
        }
    }
    else if (!rFlag && mFlag)
        raler (0, "Fournissez au moins l'option -r");
    else if (c == -1 && !rFlag && !mFlag)
        raler (0, "Vous n'avez fournis que de mauvais arguments");
    else{ // !rFlag && !mFlag   // SCENARIO 2, on recupere les nombres fournis
        n = argc - 1; 
        numbersList = malloc(n);
        if (numbersList == NULL)
            raler(1, "Erreur d'allocation");
        parseStrToNumList(n, argv, numbersList);
    }

    int raison;                     // Statut du fils a l'exit
    bool isChild = false;           // Indique si un procesus est fils ou pas
    uint8_t localValue = 0;         // Valeur que va contenir le fils
    uint64_t sum = 0;               // Somme calculee par le parent

    pid_t pid; 
    uint8_t i = 0;
    while (!isChild && i < n){      // Creation des processus fils par le parent
        switch(pid = fork()){
            case -1:
                raler (0, "Erreur de fork");
                break;
            case 0:
                isChild = true;
                localValue = numbersList[i];
                break;
            default:        // On est dans le processus pere, on continue
                break;
        }
        i++;
    }
    if (rFlag || !(rFlag&mFlag))  // Programmation defensive
        free(numbersList);
   
    if (isChild){
        CHECK(usleep(localValue*1000000/RATIO));
        printf("%d\n", localValue);
        exit(localValue);
    }else{      // L'unique processus parent
        for (uint8_t i = 0; i < n; i++){             
            CHECK(wait(&raison));
            if(WIFEXITED(raison))
                sum += WEXITSTATUS(raison);
        }
        printf("%ld\n", sum);
    }

    return 0;
}
