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

/* Vérifie les appels aux fonctions qui renvoient -1 en cas d'erreur */
#define CHK_PS(v)         \
    do                    \
    {                     \
        if ((v) == -1)    \
            raler(1, #v); \
    } while (0)

/* Pour facilement râler auprès du parent */
#define CHK_CD(v)           \
    do                      \
    {                       \
        if ((v) == -1) \
            raler_au_parent(#v);   \
    } while (0)

/* Vérifie les appels à la fonction 'signal' */
#define CHK_SG(v)           \
    do                      \
    {                       \
        if ((v) == SIG_ERR) \
            raler(1, #v);   \
    } while (0)

/* Vérifie les appels à la fonction 'fflush' */
#define CHK_FL(v)           \
    do                      \
    {                       \
        if ((v) == EOF) \
            raler_au_parent(#v);   \
    } while (0)

/* Râle */
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

/* Râler spéciale du fils auprès du père */
void raler_au_parent(char *message){
    fprintf(stderr, "Error in process %d\n%s\n", getpid(), message);
    perror("");
    fflush(stderr);
    kill(getppid(), SIGHUP);
    exit(1);
}

/* Explication de l'usage du programme */
void usage(char *prog)
{
    raler(0, "usage: %s n", prog);
}

// Permission au fils de lire un octet dans le tube
volatile sig_atomic_t allow_read;
// Indication au père de continuer a écrire dans le tube
volatile sig_atomic_t cont_write;
// Au cas ou tout ne se passe pas bien dans un fils
volatile sig_atomic_t child_has_issues;         
// Le fils reçoit un signal extérieur de terminaison
volatile sig_atomic_t child_term;

/* Fonction pour gérer ces signaux */
void fct(int sig_num)
{
    switch (sig_num){
        case SIGUSR1: allow_read = 1; break;
        case SIGUSR2: cont_write = 1; break;
        case SIGHUP: child_has_issues = 1; break;
        case SIGTERM: child_term = 1; break;
        default: break;
    };
}

/* Pour facilement initialiser les 'sigaction' */ 
void init_sigaction(struct sigaction *s, int sig, void(*f)(int)){
    s->sa_handler = f;
    s->sa_flags = 0;
    CHK_PS(sigemptyset(&s->sa_mask));
    CHK_PS(sigaddset(&s->sa_mask, sig));
    CHK_PS(sigaction(sig, s, NULL));
}

/* Pour terminer proprement tous les fils */
void kill_all_childs(pid_t *pid_fils, int n_proc){
    for (int k = 0; k < n_proc; k++)
        CHK_PS(kill(pid_fils[k], SIGHUP));

    for (int k = 0; k < n_proc; k++)
        wait(NULL);
}

/* Lecture d'un octet dans le tube par le fils */
void lecture_fils(int id_fils, int tube_0){
    unsigned char c;
    int nread;

    sigset_t masque, vieux, vide;
    CHK_CD(sigemptyset(&vide));
    CHK_CD(sigemptyset(&masque));
    CHK_CD(sigaddset(&masque, SIGUSR1));

    allow_read = 0;
    for (;;){
        CHK_CD(sigprocmask(SIG_BLOCK, &masque, &vieux)); // Section critique
        while (!allow_read) 
            sigsuspend(&vide);
        allow_read = 0;
        CHK_CD(sigprocmask(SIG_SETMASK, &vieux, NULL));

        CHK_CD(nread = read(tube_0, &c, 1));
        printf("%d: %c\n", id_fils, c);
        CHK_FL(fflush(stdout));

        CHK_CD(kill(getppid(), SIGUSR2));       // Notifie le père

        if(child_term){        // Le fils reçoit SIGTERM
            CHK_CD(kill(getppid(), SIGUSR2));   // Débloquer le père
            CHK_PS(kill(getppid(), SIGHUP));   // Terminer tout le monde
            exit(1);
        }
    }
}

/* Main */
int main(int argc, char *argv[])
{
    int n;

    if (argc != 2)
        usage(argv[0]);

    if ((n = atoi(argv[1])) <= 0)
        raler(0, "n > 0");

    // Le signal SIGUSR1 indique au fils que c'est son tour de lire
    struct sigaction usr1;
    init_sigaction(&usr1, SIGUSR1, fct);

    // Le signal SIGUSR2 indique au père qu'il peut continuer a écrire 
    struct sigaction usr2;
    init_sigaction(&usr2, SIGUSR2, fct);

    // Lorsque le fils se termine par SIGTERM, il notifie le père
    struct sigaction term;
    init_sigaction(&term, SIGTERM, fct);

    int tube[2];
    CHK_PS(pipe(tube));

    pid_t pid;
    pid_t pid_fils[n];
    for (int k = 0; k < n; k++){
        switch (pid = fork()){
            case -1:
                raler(1, "Fork");
                break;
            case 0:
                CHK_PS(close(tube[1]));
                lecture_fils(k, tube[0]);
                CHK_PS(close(tube[0]));
                exit(0);
                break;
            default:
                pid_fils[k] = pid;
                break;
        }
    }
    CHK_PS(close(tube[0]));

    // Le signal SIGHUP indique a tous les fils de s'arrêter
    struct sigaction hup;
    init_sigaction(&hup, SIGHUP, fct);
    
    init_sigaction(&term, SIGTERM, SIG_DFL);        // Retour a la normale

    sigset_t masque, vieux, vide;
    CHK_PS(sigemptyset(&vide));
    CHK_PS(sigemptyset(&masque));
    CHK_PS(sigaddset(&masque, SIGUSR2));

    int id_fils;        // Identité du fils, correspondant a k
    unsigned char buffer[TAILLE_BLOC];

    cont_write = 0;
    child_has_issues = 0;

    ssize_t n_read;
    while((n_read = read(0, buffer, TAILLE_BLOC)) > 0){
        for (int i = 0; i < n_read; i++){
            id_fils = i % n;

            CHK_PS(write(tube[1], &buffer[i], 1));
            CHK_PS(kill(pid_fils[id_fils], SIGUSR1));   // Notifie le fils

            CHK_PS(sigprocmask(SIG_BLOCK, &masque, &vieux)); // Section critique
            while (!cont_write)    
                sigsuspend(&vide);
            cont_write = 0;
            CHK_PS(sigprocmask(SIG_SETMASK, &vieux, NULL));

            if(child_has_issues){
                CHK_PS(close(tube[1]));
                kill_all_childs(pid_fils, n);
                exit(1);
            }
        }
    }

    CHK_PS(close(tube[1]));
    kill_all_childs(pid_fils, n);
    return 0;
}