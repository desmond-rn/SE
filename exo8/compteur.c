#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>


#define CHEMIN_MAX 512
#define MAX_INT_LEN 20      // Le plus grand entier possede 20 characteres

// Verifie les appels aux primitives systemes
#define CHK_PS(v)         \
    do {                  \
        if ((v) == -1)    \
            raler(1, #v); \
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

volatile sig_atomic_t sig_alrm;
volatile sig_atomic_t sig_int;
volatile sig_atomic_t sig_term;

void fct ( int sig ){
    printf("I was murdered!\n");
    exit(1);
}

void fct_alrm ( int sig ){
    sig_alrm = 1;
}

void fct_int ( int sig ){
    sig_int = 1;
}

void fct_term ( int sig ){
    sig_term = 1;
}

int main(int argc, char *argv[]) {

    if (argc != 4)
        raler(0, "Nombre d'arguments incorect");

    if (strlen(argv[1]) > CHEMIN_MAX)
        raler(0, "Chemin trop long");

    int tinc = atoi(argv[2]);
    int tstamp = atoi(argv[3]);
    if (tinc < 0 || tstamp <= 0)
        raler(0, "Temps invalide");

    alarm(tstamp);


    // signal(SIGTERM, fct);
    // kill(getpid(), SIGTERM);

    int fd = open(argv[1], O_WRONLY|O_CREAT|O_TRUNC, 0666);
    // printf("%d\n", fd);


    signal(SIGALRM, fct_alrm);
    signal(SIGINT, fct_int);
    signal(SIGTERM, fct_term);
    // kill(getpid(), SIGALRM);

    // printf("%s", time_formated);


    // CHK_PS(write(fd, time_formated, 26));
    
    time_t time_epoch;
    char *time_formated;

    char *compteur_str;
    int write_size;

    sig_alrm = 0;
    sig_int = 0;
    sig_term = 0;
    
    for(size_t compteur = 0; ; compteur++){
        usleep(tinc*1000);
        // usleep(tinc*1);

        // if (compteur%(tstamp*1000) == 0)
        //     kill(getpid(), SIGALRM);
        if(sig_alrm){
            time_epoch = time(NULL);
            time_formated = ctime(&time_epoch);
            // printf("%ld", strlen(time_formated));
            write(fd, time_formated, 25);
            sig_alrm = 0;
            alarm(tstamp);
        }

        if(sig_int){
            // add_on_int = 
            write_size = snprintf(compteur_str, MAX_INT_LEN+1, "%zu%c", compteur, '\n');
            write(fd, compteur_str, write_size);
            sig_int = 0;
        }

        if(sig_term){
            write(fd, "fin\n", 4);
            close(fd);
            return SIGTERM;
        }

        // usleep(tinc*1000);

    }

}