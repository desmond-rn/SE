#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Fonction qui effectue le codage Cesar de chaque caractere
int shift(int c, int decalage, int debut)
{
    // On ajoute 26 pour les cas ou "(c-debut) + decalage" est negatif
    int coded_c = ((c - debut) + decalage + 26) % 26 + debut;
    return coded_c;
}

int main(int argc, char* argv[])
{

    int c; // Caractere a coder
    int coded_c; // Valeur codee du caactere
    int printed_c; // Caractere imprime sur la sortie standard

    if (argc <= 1 || argc >= 4) {

        fprintf(stderr, "Erreur: Nombre d'argument incorrect");
        return 1;
    }

    // Conversion du 1er argument en entier
    int decalage;
    int parse_dclg = sscanf(argv[1], "%d", &decalage);
    if (parse_dclg == 0 || parse_dclg == EOF) {
        fprintf(stderr, "Erreur: Conversion du decalage impossible");
        return 2;
    }
    if (decalage < -25 || decalage > 25) {
        fprintf(stderr, "Erreur: Decalage non permis");
        return 2;
    }

    if (argc == 2) {

        c = getchar();
        while (!feof(stdin)) {
            if (c == EOF) { // getchar() retourne EOF sans etre a la fin
                fprintf(stderr, "Erreur: Probleme de lecture du caratere");
                return 3;
            } else {

                if (65 <= c && c <= 90) {
                    coded_c = shift(c, decalage, 65);
                } else if (97 <= c && c <= 122) {
                    coded_c = shift(c, decalage, 97);
                } else {
                    coded_c = c;
                }

                printed_c = putchar(coded_c);
                if (printed_c == EOF) {
                    fprintf(stderr, "Erreur: Probleme d'ecriture du caratere");
                    return 4;
                }
            }
            c = getchar();
        }

    } else { // argc == 3

        int n;
        int parse_n = sscanf(argv[2], "%d", &n);
        if (parse_n == EOF || parse_n == 0) {
            fprintf(stderr, "Erreur: Conversion de n impossible");
            return 5;
        }
        if (n <= 0) {
            fprintf(stderr, "Erreur: Taille du bloc non valide");
            return 5;
        }

        unsigned char bloc[n];
        int read_size = n;
        int written_size;

        while (read_size == n) {
            read_size = read(0, bloc, n);
            if (read_size == -1) {
                fprintf(stderr, "Erreur: Probleme lors de la lecture du bloc");
                perror("Details");
                return 6;
            }

            for (int i = 0; i < read_size; i++) {
                c = bloc[i];
                if (65 <= c && c <= 90) {
                    coded_c = shift(c, decalage, 65);
                } else if (97 <= c && c <= 122) {
                    coded_c = shift(c, decalage, 97);
                } else {
                    coded_c = c;
                }
                bloc[i] = coded_c;
            }

            written_size = write(1, bloc, read_size);
            if (written_size == -1) {
                fprintf(stderr, "Erreur: Probleme lors de l'ecriture du bloc");
                perror("Details");
                return 7;
            }
        }
    }

    return 0;
}
