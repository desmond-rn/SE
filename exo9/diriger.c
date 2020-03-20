#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define CHEMIN_MAX 512
#define MAX_INT_LEN 20 // Le plus grand entier possède 20 charactères

// Vérifie les appels aux fonctions qui renvoient -1 en cas d'erreur
#define CHK_PS(v)         \
    do                    \
    {                     \
        if ((v) == -1)    \
            raler(1, #v); \
    } while (0)

// Vérifie les appels à la finction 'signal'
#define CHK_SG(v)           \
    do                      \
    {                       \
        if ((v) == SIG_ERR) \
            raler(1, #v);   \
    } while (0)

// Rale
noreturn void raler(int syserr, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr)
        perror("");
    exit(1);
}

// Explication de l'usage du program
void usage(char *prog)
{
    raler(0, "usage: %s fichier tinc tstamp", prog);
}

// Valeurs atomiques pour gèrer les signaux
volatile sig_atomic_t sig_alrm, sig_int, sig_term;

void fct_alrm(int sig)
{
    (void)sig;
    sig_alrm = 1;
}

int main(int argc, char *argv[])
{
 

    return 0;
}