#!/bin/sh

PROG=${PROG:-./diriger}			# nom du programme à tester
DIR=/tmp/test9				# répertoire temporaire
# DIR=./test9				# répertoire temporaire

octets5 ()
{
    echo "abcd"				# plus le \n de echo
}

octets10 ()
{
    echo "abcdefghi"
}

# $1 = nb d'exemplaires de la chaîne de 62 octets (= alphabet)
bcp_octets ()
{
    local n=$1
    local i=0 s=""
    local alphabet="abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"

    while [ $i -lt $n ]
    do
	s="${s}${alphabet}"
	i=$((i+1))
    done
    echo "$s"
}

# génère une sortie de référence
ref ()
{
    local nproc=$1
    local c i ligne
    local newline

    newline="
"

    i=0
    while read ligne
    do
	for c in $(echo "$ligne" | sed 's/./& /g') "$newline"
	do
	    echo "$i: $c"
	    i=$(( (i + 1)%nproc ))
	done
    done
}

creer_rep ()
{
    # créer un répertoire vide
    sudo rm -rf $DIR
    mkdir $DIR
}


# tester l'analyse des arguments
test1 ()
{
    creer_rep

    # tester le nombre d'arguments
    $PROG 2> $DIR/out			&& echo "test1 failed: 1" >&2 && return
    $PROG 1 2 2> $DIR/out		&& echo "test1 failed: 2" >&2 && return

    # tester des arguments invalides
    $PROG -1 2> $DIR/out		&& echo "test1 failed: 3" >&2 && return
    $PROG 0 2> $DIR/out			&& echo "test1 failed: 4" >&2 && return

    # si on arrive ici, c'est que tout est ok
    echo "test1 ok" >&2
}

# tester exécution simple : 5 octets sur 3 processus
test2 ()
{
    creer_rep
    octets5 | $PROG 3 | sort > $DIR/moi
    octets5 | ref 3 | sort > $DIR/ref

    if diff -q $DIR/ref $DIR/moi
    then echo "test2 ok" >&2
    else echo "test2 failed: output differs from reference" >&2
    fi
}

# beaucoup d'octets sur peu de processus => lent
test3 ()
{
    creer_rep
    bcp_octets 10000 | $PROG 3 | sort > $DIR/moi
    bcp_octets 10000 | ref 3 | sort > $DIR/ref

    if diff -q $DIR/ref $DIR/moi
    then echo "test3 ok" >&2
    else echo "test3 failed: output differs from reference" >&2
    fi
}

# beaucoup d'octets sur beaucoup de processus => lent
test4 ()
{
    creer_rep
    bcp_octets 10000 | $PROG 100 | sort > $DIR/moi
    bcp_octets 10000 | ref 100 | sort > $DIR/ref

    if diff -q $DIR/ref $DIR/moi
    then echo "test4 ok" >&2
    else echo "test4 failed: output differs from reference" >&2
    fi
}

# un vrai fichier (même si on le constitue nous-mêmes)
test5 ()
{
    local i

    creer_rep

    i=0
    while [ $i -lt 100 ]
    do
	bcp_octets 1
	i=$((i+1))
    done > $DIR/fichier

    $PROG 3 < $DIR/fichier | sort > $DIR/moi
    ref 3 < $DIR/fichier | sort > $DIR/ref

    if diff -q $DIR/ref $DIR/moi
    then echo "test5 ok" >&2
    else echo "test5 failed: output differs from reference" >&2
    fi
}

# tester en présence de forte charge
test6 ()
{
    local charge=16			# 16 processus en parallèle
    local i
    local liste_pid=""

    creer_rep

    i=0
    while [ $i -lt $charge ]
    do
	(
	    while true
	    do
		: # rien du tout, c'est juste pour faire chauffer le ventilateur
	    done
	) &
	liste_pid="$liste_pid $!"
	i=$((i+1))
    done

    # laisser le temps au ventilateur de démarrer...
    sleep 1

    ( octets5 | $PROG 3 | sort > $DIR/moi ) &
    pid_pipe=$!

    # normalement, c'est très rapide : 1 seconde suffit largement pour terminer
    sleep 1

    # ok, on arrête de jouer les enfants...
    kill -TERM $liste_pid

    # si le processus "diriger" est toujours là, le test a échoué
    if kill -0 $pid_pipe 2> /dev/null
    then
	echo "test6 failed: killed" >&2
	return
    fi

    octets5 | ref 3 | sort > $DIR/ref

    if diff -q $DIR/ref $DIR/moi
    then echo "test6 ok" >&2
    else echo "test6 failed: output differs from reference" >&2
    fi
}


test1
test2
test3
test4
test5
test6
