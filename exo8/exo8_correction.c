#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <signal.h>
#include <time.h>

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
    raler (0, "usage: %s fichier tinc tstamp", prog) ;
}

volatile sig_atomic_t recu_sigint = 0 ;
volatile sig_atomic_t recu_sigalrm = 0 ;
volatile sig_atomic_t recu_sigterm = 0 ;

void f (int signum)
{
    switch (signum)
    {
	case SIGINT  : recu_sigint = 1  ; break ;
	case SIGALRM : recu_sigalrm = 1 ; break ;
	case SIGTERM : recu_sigterm = 1 ; break ;
	default :
	    raler (0, "réception du signal %d non prévu", signum) ;
    }
}

int main (int argc, char *argv [])
{
    int tinc, tstamp ;
    int compteur ;
    FILE *fp ;

    if (argc != 4)
	usage (argv [0]) ;
    tinc = atoi (argv [2]) ;
    tstamp = atoi (argv [3]) ;
    if (tinc < 0 || tstamp <= 0)
	raler (0, "tinc >= 0 et tstamp > 0") ;

    if (signal (SIGINT, f) == SIG_ERR)
	raler (1, "signal SIGINT") ;
    if (signal (SIGALRM, f) == SIG_ERR)
	raler (1, "signal SIGALRM") ;
    if (signal (SIGTERM, f) == SIG_ERR)
	raler (1, "signal SIGTERM") ;

    fp = fopen (argv [1], "w") ;
    if (fp == NULL)
	raler (1, "fopen %s", argv [1]) ;

    compteur = 0 ;
    alarm (tstamp) ;

    for (;;)
    {
	compteur++ ;
	usleep (tinc * 1000) ;

	if (recu_sigalrm)
	{
	    time_t t ;
	    t = time (NULL) ;
	    fputs (ctime (&t), fp) ;
	    alarm (tstamp) ;
	    recu_sigalrm = 0 ;
	}

	if (recu_sigint)
	{
	    fprintf (fp, "%d\n", compteur) ;
	    recu_sigint = 0 ;
	}

	if (recu_sigterm)
	{
	    fprintf (fp, "fin\n") ;
	    exit (0) ;
	}
    }
}
