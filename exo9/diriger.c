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

// Pour facilement raler aupres du parent
#define CHK_CD(v)           \
    do                      \
    {                       \
        if ((v) == -1) \
            raler_au_parent(#v);   \
    } while (0)

// Vérifie les appels à la finction 'signal'
#define CHK_SG(v)           \
    do                      \
    {                       \
        if ((v) == SIG_ERR) \
            raler(1, #v);   \
    } while (0)

// Vérifie les appels à la finction 'fflush'
#define CHK_FL(v)           \
    do                      \
    {                       \
        if ((v) == EOF) \
            raler_au_parent(#v);   \
    } while (0)

// Rale
noreturn void raler(int syserr, const char *fmt, ...)       // Doit faire kill all childs avant d'exit
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

void raler_au_parent(char *message){
    fprintf(stderr, "Error in process %d\n%s\n", getpid(), message);
    perror("");
    fflush(stderr);     // Le EOF importe peu!
    if (kill(getppid(), SIGTERM) != -1)
        exit(1);    // Sinon le fils continue neanmoins
}

// Explication de l'usage du program
void usage(char *prog)
{
    raler(0, "usage: %s n", prog);
}

// Pour que le fils lise un octet dans le tube
volatile sig_atomic_t allow_read;
// Pour que le pere continu a ecrire dans le tube
volatile sig_atomic_t cont_write;
// Au cas ou tout ne se passe pas bien dans le fils
volatile sig_atomic_t child_has_issues;         //changer en someone_has_issues pour inclure le pere

void fct(int sig_num)
{
    switch (sig_num){
        case SIGUSR1: allow_read = 1; break;
        case SIGUSR2: cont_write = 1; break;
        case SIGTERM: child_has_issues = 1; break;
        default: break;
    };
}

// Pour facilement initialiser les sigaction 
void init_sigaction(struct sigaction *s, int sig, void(*f)(int)){
    s->sa_handler = f;
    s->sa_flags = 0;
    CHK_PS(sigemptyset(&s->sa_mask));
    CHK_PS(sigaddset(&s->sa_mask, sig));
    CHK_PS(sigaction(sig, s, NULL));
}

// Pour terminer proprement tous les fils
void kill_all_childs(pid_t *pid_fils, int n_proc){
    for (int k = 0; k < n_proc; k++){
        CHK_PS(kill(pid_fils[k], SIGTERM));
    }

    // Attendre les fils
    for (int k = 0; k < n_proc; k++){
        wait(NULL);
    }
}


void lecture_fils(int id_fils, int tube_0){
    unsigned char c;
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

    // sigset_t tilde_usr1;    // Tout saus SIGUSR1
    // sigfillset(&tilde_usr1);
    // sigdelset(&tilde_usr1, SIGUSR1);
    int nread;
    sigset_t masque, vieux, vide;
    CHK_CD(sigemptyset(&vide));
    CHK_CD(sigemptyset(&masque));
    // sigemptyset(&vide);
    CHK_CD(sigaddset(&masque, SIGUSR1));
    allow_read = 0;
    for (;;){
        CHK_CD(sigprocmask(SIG_BLOCK, &masque, &vieux)); // Section critique
        while (!allow_read) 
            sigsuspend(&vide);
        allow_read = 0;
        CHK_CD(sigprocmask(SIG_SETMASK, &vieux, NULL));

        // read(tube_0, &c, 1);     // Section critique
        // printf("mon tour de lire dans %d\n", tube_0);
        // CHK_PS(nread = read(tube_0, &c, 5));
        CHK_CD(nread = read(tube_0, &c, 1));
        // CHK_FL(fflush(stderr));
        printf("%d: %c\n", id_fils, c);
        CHK_FL(fflush(stdout));
        CHK_CD(kill(getppid(), SIGUSR2));
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
    // usr1.sa_handler = fct;
    // usr1.sa_flags = 0;
    // sigemptyset(&usr1.sa_mask);
    // sigaddset(&usr1.sa_mask, SIGUSR1);
    // sigaction(SIGUSR1, &usr1, NULL);
    init_sigaction(&usr1, SIGUSR1, fct);

    // Pour le pere
    struct sigaction usr2;
    // usr2.sa_handler = fct;
    // usr2.sa_flags = 0;
    // sigemptyset(&usr2.sa_mask);
    // sigaddset(&usr2.sa_mask, SIGUSR2);
    // sigaction(SIGUSR2, &usr2, NULL);
    init_sigaction(&usr2, SIGUSR2, fct);


    int tube[2];
    CHK_PS(pipe(tube));

    pid_t pid;
    pid_t pid_fils[n];

    for (int k = 0; k < n; k++){        // Utiliser k plutot
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
            CHK_PS(close(tube[1]));
            
            lecture_fils(k, tube[0]);

            CHK_PS(close(tube[0]));
            exit(0);
            break;

        default:
            // close(tube[0]);
            pid_fils[k] = pid;
            break;
        }
    }

    CHK_PS(close(tube[0]));

    // Seul le pere possede cette implementation
    struct sigaction term;
    init_sigaction(&term, SIGTERM, fct);

    // Masque vide pour le sigsuspend, et le sigprocmask
    sigset_t masque, vieux, vide;
    CHK_PS(sigemptyset(&vide));
    CHK_PS(sigemptyset(&masque));
    // sigemptyset(&vide);
    CHK_PS(sigaddset(&masque, SIGUSR2));

    int id_fils;        // Identite du fils, correspondant a k
    unsigned char buffer[TAILLE_BLOC];

    // Lecture par bloc 
    // size_t nrem = TAILLE_BLOC;


    cont_write = 0; // Le pere commence a ecrire dans le tube par defaut
    child_has_issues = 0;
    size_t n_read;
    while((n_read = read(0, buffer, TAILLE_BLOC)) > 0){
        for (int i = 0; i < n_read; i++){
            id_fils = i % n;

            // printf("\nID DU FILS %d\n", pid_fils[id_fils]);

            // kill(pid_fils[id_fils], SIGUSR1);

            // for (; ;){

            CHK_PS(write(tube[1], &buffer[i], 1));
            CHK_PS(kill(pid_fils[id_fils], SIGUSR1));

            CHK_PS(sigprocmask(SIG_BLOCK, &masque, &vieux)); // Section critique
            while (!cont_write)    
                sigsuspend(&vide);
            cont_write = 0;
            CHK_PS(sigprocmask(SIG_SETMASK, &vieux, NULL));

            // CHK_PS(write(tube[1], &buffer[i], 1));
            // kill(pid_fils[id_fils], SIGUSR1);

            // char buf[];
            // printf("\nMON C %c\n", buffer[i]);

            if(child_has_issues){
                CHK_PS(close(tube[1]));
                kill_all_childs(pid_fils, n);
                // CHK_FL(fflush(stdout));
                exit(1);
            }

                // break;
            // }

        }
    }

    // close(tube[1]);

    // // Si l'entree standard est vide
    // for (int k = 0; k < n; k++){
    //     kill(pid_fils[k], SIGHUP);
    // }

    // // Attendre les fils
    // for (int k = 0; k < n; k++){
    //     wait(NULL);
    // }
    // printf(" ");
    // fprintf(1, " ");
    // fflush(stdout);
    CHK_PS(close(tube[1]));
    kill_all_childs(pid_fils, n);
    // CHK_FL(fflush(stdout));
    return 0;
    }