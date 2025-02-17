#!/bin/sh

PROG=${PROG:-./arbexec2}		# nom du programme à tester
DIR=/tmp/test7				# répertoire temporaire

# $1 = nb de lignes souhaité
nlignes ()
{
    local n="$1"
    head -n $n /usr/include/stdio.h
}

creer_arb_lignes ()
{
    rm -rf $DIR
    mkdir $DIR $DIR/d
    nlignes 5 > $DIR/d/cinq
    nlignes 10 > $DIR/d/dix
    mkdir $DIR/d/sub
    nlignes 2 > $DIR/d/sub/deux
    nlignes 3 > $DIR/d/sub/trois
    
}

# tester l'analyse des arguments
test1 ()
{
    # créer un répertoire vide
    rm -rf $DIR
    mkdir $DIR $DIR/d

    # tous ces tests doivent fonctionner (code de retour = 0)
    $PROG $DIR/d wc > $DIR/out         || (echo "test1 failed: 1" >&2 && return)
    $PROG $DIR/d wc -l > $DIR/out      || (echo "test1 failed: 2" >&2 && return)

    # aucun de ces tests ne doit fonctionner (code de retour != 0)
    $PROG $DIR/d 2> $DIR/out           && echo "test1 failed: 3" >&2 && return

    # on ne teste pas "--" car on se dispense de l'utilisation de getopt

    # si on arrive ici, c'est que tout est ok
    echo "test1 ok" >&2
}

# tester exécution simple
test2 ()
{
    local file

    creer_arb_lignes
    # tester sur l'arborescence (et trier pour normaliser la sortie)
    $PROG $DIR/d wc -l | sort > $DIR/out.moi

    # déterminer la référence
    find $DIR/d -type f \
	| while read file
	    do
		wc -l $file
	    done | sort > $DIR/out.ref

    if diff -q $DIR/out.ref $DIR/out.moi
    then echo "test2 ok" >&2
    else echo "test2 failed: output differs from reference" >&2
    fi
}

# tester lancement en parallèle (test long : ~5 secondes)
test3 ()
{
    local debut fin duree

    rm -rf $DIR
    mkdir $DIR
    (
	echo '#!/bin/sh'
	echo 'sleep $(cat $1)    # attendre la durée specifiée ds le fichier'
    ) > $DIR/cmd
    chmod +x $DIR/cmd
    mkdir $DIR/d $DIR/d/d1 $DIR/d/d2
    echo 2 > $DIR/d/d1/deux
    echo 3 > $DIR/d/d1/trois
    echo 5 > $DIR/d/d2/cinq
    echo 4 > $DIR/d/d2/quatre

    # test3 : doit durer 5 (= max(2,3,5,4)) secondes
    debut=$(date +%s)		# date +%s n'est pas POSIX, mais ok sur Linux
    $PROG $DIR/d $DIR/cmd 
    fin=$(date +%s)
    duree=$((fin-debut))

    # tolérance de +/- 1 seconde
    if [ $duree -ge 4 -a $duree -le 6 ]
    then echo "test3 ok" >&2
    else echo "test3 failed: durée $duree, devrait être 5 secondes" >&2
    fi
}

# tester le code de retour si un fils se termine mal
test4 ()
{
    local file exitcode

    # création d'une commande qui retourne un code de retour non nul
    # seulement pour le fichier "deux"
    rm -rf $DIR
    mkdir $DIR
    (
	echo '#!/bin/sh'
	echo 'R=0 ; if [ $(basename $1) = deux ] ; then R=1 ; fi'
	echo 'exit $R'
    ) > $DIR/cmd
    chmod +x $DIR/cmd

    # on n'a besoin que d'un seul fichier
    mkdir $DIR/d
    echo 1 > $DIR/d/un
    echo 2 > $DIR/d/deux
    echo 3 > $DIR/d/trois

    $PROG $DIR/d $DIR/cmd > $DIR/out.1 2> $DIR/out.2
    exitcode=$?				# code retour de la dernière commande

    # on attend un code de retour non nul
    if [ $exitcode != 0 ]
    then echo "test4 ok" >&2
    else echo "test4 failed (exit code = $exitcode)" >&2
    fi
}

# on aime bien tester sur des grands volumes
test5 ()
{
    rm -rf $DIR
    mkdir $DIR

    $PROG /usr/include wc -l | sort > $DIR/out.moi

    # déterminer la référence
    find /usr/include -type f \
	| while read file
	    do
		wc -l $file
	    done | sort > $DIR/out.ref

    if diff -q $DIR/out.ref $DIR/out.moi
    then echo "test5 ok" >&2
    else echo "test5 failed: output differs from reference" >&2
    fi
}

test1
test2
test3
test4
test5
