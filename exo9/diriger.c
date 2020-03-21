#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>

#define CHEMIN_MAX 512
#define TAILLE_BLOC 4096

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
    raler(0, "usage: %s n", prog);
}

// Pour que le fils lise un octet dans le tube
volatile sig_atomic_t allow_read;
// Pour que le pere continu a ecrire dans le tube
volatile sig_atomic_t cont_write = 1;

void fct(int sig_num)
{
    switch (sig_num){
        case SIGUSR1: allow_read = 1; break;
        case SIGUSR2: cont_write = 1; break;
        default: break;
    };
}

void fct1(int sig_num)
{
    allow_read = 1;
}

void fct2(int sig_num)
{
    cont_write = 1;
}

void lecture_fils(int id_fils, int tube_0){
    char c[10];
    // for(;;){
    //     if (allow_read){
    //         read(tube_0, &c, 1);
    //         printf("%d : %c\n", id_fils, c);
    //         // printf("Mon tour de lire dans %d\n", tube_0);
    //         fflush(stdout);
    //         allow_read = 0;
    //         kill(getppid(), SIGUSR2);
    //     }
    // }

    sigset_t vide;
    sigemptyset(&vide);
    // sigaddset(&vide, SIGUSR1);
    for (;;){
        if (!allow_read)    // Section critique, aussi le programme ne peut pas exiter dans cette section
            sigsuspend(&vide);

        int nread;
        // read(tube_0, &c, 1);     // Section critique
        // printf("mon tour de lire dans %d\n", tube_0);
        // CHK_PS(nread = read(tube_0, &c, 5));
        if((nread = read(tube_0, &c, 10)) == -1){
            fprintf(stderr, "process %d ", id_fils);
            perror("");
            ;
        }
        fflush(stderr);
        printf("%d : %c\n", id_fils, c[0]);
        fflush(stdout);
        allow_read = 0;
        kill(getppid(), SIGUSR2);
    }


}


// Faire un while infini et un suspend pour les fils
int main(int argc, char *argv[])
{
    int n;

    if (argc != 2)
        usage(argv[0]);

    if ((n = atoi(argv[1])) <= 0)
        raler(0, "n > 0");

    /* Penser a une boucle pour faire ca */
    // Pour le fils
    struct sigaction usr1;
    usr1.sa_handler = fct;
    usr1.sa_flags = 0;
    sigemptyset(&usr1.sa_mask);
    sigaddset(&usr1.sa_mask, SIGUSR1);
    sigaction(SIGUSR1, &usr1, NULL);

    // Pour le pere
    struct sigaction usr2;
    usr2.sa_handler = fct;
    usr2.sa_flags = 0;
    sigemptyset(&usr2.sa_mask);
    sigaddset(&usr2.sa_mask, SIGUSR2);
    sigaction(SIGUSR2, &usr2, NULL);

    int tube[2];
    CHK_PS(pipe(tube));

    __pid_t pid;
    __pid_t pid_fils[n];

    for (int i = 0; i < n; i++){        // Utiliser k plutot
        // if(pipe(tube) == -1)
        //     fprintf(stderr, "Error pipe\n");
        // fflush(stderr);
        switch (pid = fork())
        {
        case -1:
            raler(1, "Fork");
            break;
        case 0:
            // printf("\nTUBE: %d et %d\n", tube[0], tube[1]);
            close(tube[1]);
            
            lecture_fils(i, tube[0]);

            close(tube[0]);
            exit(0);
            break;

        default:
            pid_fils[i] = pid;
            break;
        }
    }

    close(tube[0]);

    // cont_write = 1;

    sigset_t vide;
    sigemptyset(&vide);

    int id_fils;        // Changer le nom en k
    char buffer[TAILLE_BLOC];
    int n_read = read(0, buffer, TAILLE_BLOC);

    for (int i = 0; i < n_read; i++){
        id_fils = i%n;

        // printf("\nID DU FILS %d\n", pid_fils[id_fils]);

        kill(pid_fils[id_fils], SIGUSR1);

        for (; ;){
            
            // sigsuspend(&vide);
            
            if (cont_write){    //section critique
                CHK_PS(write(tube[1], &buffer[i], 1));
                
                // char buf[];
                // printf("\nMON C %c\n", buffer[i]);
                
                cont_write = 0;
                break;
            }
        }

    }

    close(tube[1]);

    // Si l'entree standard est vide
    for (int i = 0; i < n; i++){
        kill(pid_fils[i], SIGHUP);
    }

    // Attendre les fils
    for (int i = 0; i < n; i++){
        wait(NULL);
    }


        return 0;
    }