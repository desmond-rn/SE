#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define VAL_MAX 255  // Valeur maximale des nombres
#define RATIO 10     // Rapport variable/temps pris pour l'affichier

// Verifie les appels aux primitives systemes
#define CHECK(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
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

// Calcule une liste de n nombre pseudo-aleatoirement entre 0 et nMax inclu
void computeRandNumList(uint8_t n, uint8_t nMax, uint8_t *numbersList) {
    srand(time(NULL));  // Changement du noyau
    for (uint8_t i = 0; i < n; i++)
        // numbersList[i] = rand()%(nMax+1);        // Pas assez random
        // On divise essentiellement rand() par RAND_MAX+1 ... (en double)
        numbersList[i] =
            (int)((double)rand() * (nMax + 1) / ((double)RAND_MAX + (double)1));
}

// Convertit une liste de chaine de charactere en liste d'entier
void parseStrToNumList(int nStr, char *strList[], uint8_t *numbersList) {
    int64_t temp;  // Pour economiser les acces memoire
    for (uint8_t i = 0; i < nStr; i++) {
        temp = atoi(strList[i + 1]);
        if (temp <= 0 || temp > VAL_MAX)
            raler(0, "Valeur non permise: '%s'", strList[i + 1]);
        numbersList[i] = temp;
    }
}

int main(int argc, char *argv[]) {
    int c;
    extern char *optarg;
    extern int optind;
    bool errFlag = false;

    bool rFlag = false;    // Indique si l'option r a ete donnee ou pas
    bool mFlag = false;    // Indique si l'option m a ete fournie
    int64_t n, nMax;       // 64 bits pour detecter les grosses valeurs
    uint8_t *numbersList;  // La liste des nombres a trier

    if (argc == 1) raler(0, "Precisez une syntaxe pour le programme");

    while ((c = getopt(argc, argv, ":rm:")) != -1) {
        switch (c) {
            case 'r':
                rFlag = true;  // On peut fournir l'option -r plusieurs fois
                break;
            case 'm':  // On peut fournir l'option -m plusieurs fois
                mFlag = true;
                nMax = atoi(optarg);
                break;
            case ':':
                raler(0, "Fournissez l'argument de -m");
                break;
            case '?':
                raler(0, "Option inconnue: %s", argv[optind - 1]);
                break;
            default:
                errFlag = true;
                break;
        }
        if (errFlag) raler(0, "Erreur dans 'getopt'");
    }

    if (rFlag) {  // SCENARIO 1, on calcule aleatoirement les nombres
        if (optind > argc - 1)  // On a parcouru tous les arguments
            raler(0, "Fournissez l'argument de -r");
        else if (optind < argc - 1)  // 'getopt' s'est arrete trop tot
            raler(0, "Vous avez fournis trop d'arguments");
        else {  // optind == argc - 1
            n = atoi(argv[optind]);
            if (n <= 0 || n > VAL_MAX) raler(0, "Valeur non permise pour 'n'");
            numbersList = malloc(n);
            if (numbersList == NULL) raler(1, "Erreur d'allocation de memoire");
            if (!mFlag)
                computeRandNumList(n, n, numbersList);
            else {  // mFlag == true
                if (nMax <= 0 || nMax > VAL_MAX)
                    raler(0, "Valeur non permise pour 'nmax'");
                computeRandNumList(n, nMax, numbersList);
            }
        }
    } else if (!rFlag && mFlag)
        raler(0, "Fournissez l'option -r");
    else if (!rFlag && !mFlag) {  // SCENARIO 2, on recupere les nombres fournis
        n = argc - 1;
        numbersList = malloc(n);
        if (numbersList == NULL) raler(1, "Erreur d'allocation de memoire");
        parseStrToNumList(n, argv, numbersList);
    }

    int raison;              // Statut du 'fils' a l'exit
    bool isChild = false;    // Indique si un procesus est fils ou pas
    uint8_t localValue = 0;  // Valeur a trier que va contenir le fils
    uint64_t sum = 0;        // Somme calculee par le parent
    pid_t pid;

    uint8_t i = 0;
    while (!isChild && i < n) {  // Creation des fils seulement par le parent
        switch (pid = fork()) {
            case -1:
                raler(0, "Erreur de fork");
                break;
            case 0:
                isChild = true;
                localValue = numbersList[i];
                break;
            default:  // On est dans le processus pere, on continue
                break;
        }
        i++;
    }

    if (rFlag || !(rFlag & mFlag))  // Programmation defensive
        free(numbersList);

    if (isChild) {
        CHECK(usleep(localValue * 1000000 / RATIO));
        printf("%d\n", localValue);
        exit(localValue);
    } else {  // L'unique processus parent
        for (uint8_t i = 0; i < n; i++) {
            CHECK(wait(&raison));
            if (WIFEXITED(raison)) sum += WEXITSTATUS(raison);
        }
        printf("%ld\n", sum);
    }

    return 0;
}
