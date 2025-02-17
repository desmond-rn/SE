#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <inttypes.h>			// uint8_t uint16_t uint32_t uint64_t
#include <sys/stat.h>
#include <utime.h>
#include <stdarg.h>
#include <stdnoreturn.h>

#define CHK(op)	do { if ((op)==-1) raler (1, #op) ; } while (0)

// lit une variable dans l'archive
#define	ARCREAD(fd,v) do { int n ; \
			CHK (n = read (fd, &(v), sizeof (v))) ; \
			if (n != sizeof (v)) corrompue () ; \
		    } while (0)
// tente de lire une variable dans l'archive et retourne 0 en fin d'archive
#define	ENDREAD(fd,v) do { int n ; \
			CHK (n = read (fd, &(v), sizeof (v))) ; \
			if (n == 0) return 0 ; \
			if (n != sizeof (v)) corrompue () ; \
		    } while (0)

uint8_t magic [] = { 0x63, 0x73, 0x6d, 0x69 } ;		// ou 'c', 's', 'm', 'i'

#define	CHEMIN_MAX	512
#define	TAILLE_BLOC	4096

#define	TYPE_REG	0x01
#define	TYPE_DIR	0x02
#define	TYPE_LNK	0x03

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

/******************************************************************************
 * Utilitaires
 */

void copier_fichier (int fdsrc, int fddst, off_t taille)
{
    while (taille > 0)
    {
	char buf [TAILLE_BLOC] ;
	size_t ndem ;			// nb d'octets demandés
	ssize_t nlus ;			// nb d'octets lus

	ndem = (taille > TAILLE_BLOC) ? TAILLE_BLOC : taille ;
	CHK (nlus = read (fdsrc, buf, ndem)) ;
	CHK (write (fddst, buf, nlus)) ;
	taille -= nlus ;
    }
}

/******************************************************************************
 * Extraction
 */

void corrompue (void)
{
    raler (0, "archive corrompue") ;
}

// path est censé être un tableau de CHEMIN_MAX + 1 octets
void lire_chemin (int fdarc, char *path)
{
    uint16_t lgpath ;
    ssize_t n ;

    ARCREAD (fdarc, lgpath) ;
    if (lgpath > CHEMIN_MAX)
	raler (0, "nom trop long (%d caractères)", lgpath) ;
    CHK (n = read (fdarc, path, lgpath)) ;
    if (n != lgpath)
	corrompue () ;
    path [lgpath] = '\0' ;	// pas de '\0' terminal dans l'archive
}

int lire_entete (int fdarc, char *path, struct stat *stbuf)
{
    uint8_t type ;
    uint16_t perm ;

    ENDREAD (fdarc, type) ;
    ARCREAD (fdarc, perm) ;
    switch (type)
    {
	case TYPE_REG : stbuf->st_mode = S_IFREG | perm ; break ;
	case TYPE_DIR : stbuf->st_mode = S_IFDIR | perm ; break ;
	case TYPE_LNK : stbuf->st_mode = S_IFLNK | perm ; break ;
	default : raler (0, "type inconnu dans l'en-tete") ;
    }
    lire_chemin (fdarc, path) ;
    return 1 ;
}

void extraire_fichier (int fdarc, char *path)
{
    uint32_t mtime ;
    uint64_t taille ;
    int fd ;

    ARCREAD (fdarc, mtime) ;
    ARCREAD (fdarc, taille) ;
    CHK (fd = open (path, O_WRONLY|O_CREAT|O_TRUNC, 0666)) ;
    copier_fichier (fdarc, fd, taille) ;
    CHK (close (fd)) ;

    struct utimbuf u ;
    u.actime = u.modtime = mtime ;
    CHK (utime (path, &u)) ;
}

void extraire_lien (int fdarc, char *path)
{
    char target [CHEMIN_MAX + 1] ;

    lire_chemin (fdarc, target) ;
    CHK (symlink (target, path)) ;
}

void extraire (const char *archive)
{
    int fdarc ;
    uint8_t magic_lu [4] ;
    struct stat stbuf ;
    char path [CHEMIN_MAX + 1] ;
    int n ;

    CHK (fdarc = open (archive, O_RDONLY)) ;
    CHK (n = read (fdarc, &magic_lu, sizeof magic_lu)) ;
    if (n != sizeof magic_lu || memcmp (magic, magic_lu, sizeof magic) != 0)
	raler (0, "%s n'est pas une archive", archive) ;
    while (lire_entete (fdarc, path, &stbuf))	// arrêt en fin d'archive
    {
	switch (stbuf.st_mode & S_IFMT)
	{
	    case S_IFDIR :
		CHK (mkdir (path, 0777)) ;
		CHK (chmod (path, stbuf.st_mode & 0777)) ;
		break ;
	    case S_IFREG :
		extraire_fichier (fdarc, path) ;
		CHK (chmod (path, stbuf.st_mode & 0777)) ;
		break ;
	    case S_IFLNK :
		extraire_lien (fdarc, path) ;
		break ;
	}
    }
}

/******************************************************************************
 * Création
 */

void ecrire_entete (int fdarc, const char *path, struct stat *stbuf)
{
    uint8_t type ;
    uint16_t perm ;
    uint16_t lgpath ;

    switch (stbuf->st_mode & S_IFMT)
    {
	case S_IFREG : type = TYPE_REG ; break ;
	case S_IFDIR : type = TYPE_DIR ; break ;
	case S_IFLNK : type = TYPE_LNK ; break ;
	default : raler (0, "type inconnu") ;
    }
    perm = stbuf->st_mode & 0777 ;
    CHK (write (fdarc, &type, sizeof type)) ;
    CHK (write (fdarc, &perm, sizeof perm)) ;
    lgpath = strlen (path) ;
    CHK (write (fdarc, &lgpath, sizeof lgpath)) ;
    CHK (write (fdarc, path, lgpath)) ;
}

void ecrire_fichier (int fdarc, const char *path, struct stat *stbuf)
{
    int fd ;
    uint32_t mtime ;
    uint64_t taille ;

    mtime = stbuf->st_mtime ;
    CHK (write (fdarc, &mtime, sizeof mtime)) ;
    taille = stbuf->st_size ;
    CHK (write (fdarc, &taille, sizeof taille)) ;
    CHK (fd = open (path, O_RDONLY)) ;
    copier_fichier (fd, fdarc, taille) ;
    CHK (close (fd)) ;
}

void ecrire_lien (int fdarc, const char *path, struct stat *stbuf)
{
    uint16_t taille ;
    char target [CHEMIN_MAX] ;

    if (stbuf->st_size > CHEMIN_MAX)
	raler (0, "cible du lien %s trop long", path) ;
    taille = stbuf->st_size ;
    CHK (readlink (path, target, taille)) ;

    CHK (write (fdarc, &taille, sizeof taille)) ;
    CHK (write (fdarc, target, taille)) ;
}

void creer_rec (int fdarc, const char *dir, struct stat *stdir)
{
    struct stat stbuf ;
    DIR *dp ;
    struct dirent *d ;

    if ((dp = opendir (dir)) == NULL)
	raler (1, "opendir %s", dir) ;
    if (stdir == NULL)
    {
	CHK (stat (dir, &stbuf)) ;
	stdir = &stbuf ;
    }
    ecrire_entete (fdarc, dir, stdir) ;

    while ((d = readdir (dp)) != NULL)
    {
	if (strcmp (d->d_name, ".") != 0 && strcmp (d->d_name, "..") != 0)
	{
	    char npath [CHEMIN_MAX + 1] ;
	    int snp ;

	    snp = snprintf (npath, sizeof npath, "%s/%s", dir, d->d_name) ;
	    if (snp < 0 || snp >= (int) sizeof npath)
		raler (0, "chemin '%s/%s' trop long", dir, d->d_name) ;
	    CHK (lstat (npath, &stbuf)) ;
	    switch (stbuf.st_mode & S_IFMT)
	    {
		case S_IFDIR :
		    creer_rec (fdarc, npath, &stbuf) ;
		    break ;
		case S_IFREG :
		    ecrire_entete (fdarc, npath, &stbuf) ;
		    ecrire_fichier (fdarc, npath, &stbuf) ;
		    break ;
		case S_IFLNK :
		    ecrire_entete (fdarc, npath, &stbuf) ;
		    ecrire_lien (fdarc, npath, &stbuf) ;
		    break ;
		default :
		    raler (0, "%s : type inconnu") ;
	    }
	}
    }
}

void creer (const char *archive, const char *dir)
{
    int fdarc ;

    if (dir [0] == '/')
	raler (0, "%s est un nom absolu", dir) ;
    CHK (fdarc = open (archive, O_WRONLY|O_CREAT|O_TRUNC, 0666)) ;
    CHK (write (fdarc, magic, sizeof magic)) ;
    creer_rec (fdarc, dir, NULL) ;
    CHK (close (fdarc)) ;
}

int main (int argc, char *argv [])
{
    switch (argc)
    {
	case 3 :
	    if (strcmp (argv [1], "x") == 0)
		extraire (argv [2]) ;
	    break ;
	case 4 :
	    if (strcmp (argv [1], "c") == 0)
		creer (argv [2], argv [3]) ;
	    break ;
	default :
	    raler (0, "usage: %s c|x archive [dir]", argv [0]) ;
    }

    exit (0) ;
}
