#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define	SIGLIRE		SIGUSR1		// père -> fils
#define	SIGACK		SIGUSR1		// fils -> père (SIGUSR2, peu importe)

#define MAXBUF	512

#define CHK(op)		do { if((op)==-1) raler(1,#op); } while (0)

noreturn void raler (int syserr, const char *fmt, ...)
{
    va_list ap ;

    va_start (ap, fmt) ;
    vfprintf (stderr, fmt, ap) ;
    fprintf (stderr, "\n") ;
    va_end (ap) ;

    if (syserr)
	perror ("") ;

    exit (1) ;
}

void usage (char *prog)
{
    raler (0, "usage: %s nproc", prog) ;
}

/******************************************************************************
 * Fonctions communes
 */

void preparer_signal (int signum, void (*fct) (int))
{
    struct sigaction s ;
    sigset_t masque ;

    s.sa_handler = fct ;
    s.sa_flags = 0 ;
    CHK (sigemptyset (&s.sa_mask)) ;
    CHK (sigaction (signum, &s, NULL)) ;

    CHK (sigemptyset (&masque)) ;
    CHK (sigaddset (&masque, signum)) ;
    CHK (sigprocmask (SIG_BLOCK, &masque, NULL)) ;
}

/******************************************************************************
 * Fils
 */

void fct_siglire (int signum)
{
    (void) signum ;
}

void fils (int fdin, int moi, pid_t pid_pere)
{
    unsigned char c ;
    int nlus ;
    sigset_t masque ;

    CHK (sigemptyset (&masque)) ;

    nlus = 1 ;
    while (nlus > 0)
    {
	sigsuspend (&masque) ;

	CHK (nlus = read (fdin, &c, 1)) ;
	if (nlus > 0)			// <=> if (nlus == 1)
	{
	    printf ("%d: %c\n", moi, c) ;
	    if (fflush (stdout) == EOF)
		raler (1, "fflush") ;
	    CHK (kill (pid_pere, SIGACK)) ;
	}
    }

    exit (0) ;			// Si un fils ne lit rien, il se tues lui meme
}

/******************************************************************************
 * Père
 */

void fct_sigack (int signum)
{
    (void) signum ;
}

void pere (int fdout, pid_t tpid [], int nproc)
{
    char buf [MAXBUF] ;
    int nlus ;
    int fils ;
    sigset_t masque ;

    CHK (sigemptyset (&masque)) ;

    fils = 0 ;
    while ((nlus = read (0, buf, sizeof buf)) > 0)
    {
	for (int i = 0 ; i < nlus ; i++)
	{
	    CHK (write (fdout, &buf [i], 1)) ;
	    CHK (kill (tpid [fils], SIGLIRE)) ;
	    fils = (fils + 1) % nproc ;

	    sigsuspend (&masque) ;
	}
    }
    CHK (nlus) ;
}

/******************************************************************************
 * Main
 */

int main (int argc, char *argv [])
{
    int nproc ;
    int tube [2] ;
    int i ;
    pid_t *tpid, pid_pere ;

    if (argc != 2)
	usage (argv [0]) ;

    nproc = atoi (argv [1]) ;
    if (nproc <= 0)
	raler (0, "nproc doit être > 0") ;

    if ((tpid = calloc (nproc, sizeof *tpid)) == NULL)
	raler (1, "cannot malloc %d pid_t", nproc) ;

    CHK (pipe (tube)) ;
    pid_pere = getpid () ;

    // préparer la réception des signaux (du signal) PAR LE FILS
    preparer_signal (SIGLIRE, fct_siglire) ;

    for (i = 0 ; i < nproc ; i++)
    {
	switch (tpid [i] = fork ())
	{
	    case -1 :
		raler (1, "cannot fork") ;

	    case 0 :
		CHK (close (tube [1])) ;
		fils (tube [0], i, pid_pere) ;
		CHK (close (tube [0])) ;
		exit (1) ;

	    default :
		break ;
	}
    }

    // préparer la réception des signaux (du signal) PAR LE PÈRE
    preparer_signal (SIGACK, fct_sigack) ;

    CHK (close (tube [0])) ;
    pere (tube [1], tpid, nproc) ;
    CHK (close (tube [1])) ;

    // Astuce tres interressante pour tuer tous les fils
    for (i = 0 ; i < nproc ; i++)
	CHK (kill (tpid [i], SIGLIRE)) ;

    for (i = 0 ; i < nproc ; i++)
	CHK (wait (NULL)) ;

    exit (0) ;
}
