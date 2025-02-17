#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Verifie que les allocations de memoire se sont bien passes
 * @param Pointeur sur un caratere
 * @result void
 */
void checkDynamicAllocation(char* pointer)
{
    if (pointer == NULL) {
        printf("Error allocating memory...\n");
        exit(-1);
    }
}

int main(int argc, char* argv[])
{
    // printf("PROGRAMME DE SUBSTITUTION D'UN CARACTERE\n");

    // Verification du bon nombre d'arguments
    if (argc != 3 && argc != 4) {
        printf("Nombre d'argument a l'execution pas correct...\n");
        return -1;
    }

    // indices des arguments 'lettre' et 'remplacement' a l'execution
    int index_letter, index_replacement;
    if (argc == 3) {
        index_letter = 1;
        index_replacement = 2;
    } else {
        index_letter = 2;
        index_replacement = 3;
    }

    // Le buffer pour contenir le texte (ou les caracteres binaires)
    // avec lequel on va travailler
    char* text;

    // str_letter contient la transofmation en chaine de caratere de
    // la lettre a substituer
    char* str_letter = argv[index_letter];

    // Le mot de remplacement
    const char* replacement = argv[index_replacement];

    // Chaine de cractere pour contenir chaque charactere du texte en
    // vue de la comparaison
    char* str_text_char = malloc(2 * sizeof(char));
    checkDynamicAllocation(str_text_char);

    // La taille du fichier (texte ou binaire) fourni, necessaire pour
    // creer le buffer
    int text_size;
    if (argc == 4) {

        FILE* text_file = fopen(argv[1], "rb");
        if (text_file == NULL) {
            printf("Error opening file...\n");
            return -1;
        }

        // Determinons la taille du texte
        fseek(text_file, 0, SEEK_END);
        const int text_file_size = ftell(text_file);
        text_size = text_file_size;
        // printf("Text file size: %d\n", text_file_size);
        text = malloc(text_file_size * sizeof(char));
        checkDynamicAllocation(text);
        text[0] = '\0';

        // On retourne au debut du fichier pour la lecture
        fseek(text_file, 0, SEEK_SET);
        // int c;
        // while (!feof(text_file)){        // Pour les fichiers texte
        //     c = fgetc(text_file);
        //     // printf("%c", c);
        //     str_text_char[0] = c;
        //     str_text_char[1] = '\0';
        //     strcat(text, str_text_char);
        // }

        // Pour lire les fichiers binaires et textes
        fread(text, sizeof(char), text_file_size, text_file);

        fclose(text_file);
    }

    if (argc == 3) {

        if (strlen(argv[1]) != 1) {
            printf("Erreur, fournissez le caractere a supprimer...\n");
            return -1;
        }

        fseek(stdin, 0, SEEK_END);
        // Taille de fichier lu en entree standard
        const int input_stream_size = ftell(stdin);
        text_size = input_stream_size;

        if (input_stream_size == -1) {
            // Recupere entierement le texte saisi au clavier sauf '\n'
            // et alloue l'espace necessaire
            scanf("%m[^\n]", &text);
            checkDynamicAllocation(text);
            text_size = strlen(text);
            // printf("Erreur, fournissez un fichier (texte ou binaire)...\n");
            // return -1;
        } else {
            text = malloc(input_stream_size * sizeof(char));
            checkDynamicAllocation(text);
            text[0] = '\0';

            fseek(stdin, 0, SEEK_SET);
            // Recupere tout les caracteres mais pas correctement
            // fgets(text, input_stream_size, stdin);

            // while (!feof(stdin)){
            //     str_text_char[0] = fgetc(stdin);
            //     str_text_char[1] = '\0';
            //     strcat(text, str_text_char);
            // }

            // Pour bien lire les fichiers binaires
            fread(text, sizeof(char), input_stream_size, stdin);
        }
    }

    // printf("\n");
    // printf("Texte avant la substitution:\n\"%s\"\n\n", text);
    // // printf("Text size: %ld\n", strlen(text));
    // printf("Caractere a substituer: '%s'\n", str_letter);
    // printf("Chaine de caracteres a inserer: \"%s\"\n", replacement);

    // Taille de la chaine de caractere de remplacement
    const int replacement_size = strlen(replacement);
    // Tous les cas etant traites, taille finale du fichier a modifier
    const int old_text_size = text_size;
    // Nouvelle chaine de carateres qui va contenir le texte
    // (ou la chaine de caraterere) modifiee
    char* new_text = malloc(old_text_size * sizeof(char));
    checkDynamicAllocation(new_text);
    // Taille provisoire necessaire pour stocker le nouveau texte
    int current_new_text_size = old_text_size;

    // Taille courante du nouveau buffer contenant nos caracteres
    int new_text_size = 0;
    // La boucle 'while' commentee fonctionne correctement pour
    // les chaines de carateres
    for (int i = 0; i < old_text_size; i++) {
        // while (*text != '\0'){
        // On tranforme text[i] en chaine de carateres
        str_text_char[0] = text[i];
        // str_text_char[0] = *text;
        str_text_char[1] = '\0';
        // if (strcmp(str_letter, str_text_char) != 0){
        if (memcmp(str_letter, str_text_char, 2) != 0) {

            // strcat(new_text, str_text_char);
            memcpy(&new_text[new_text_size], &str_text_char[0], 2);

            new_text_size += 1;

        } else {
            // On ajoute la taille de new_text pour pouvoir contenir la
            // taille du remplacement
            // if(replacement_size != 1){
            current_new_text_size += (replacement_size - 1);
            new_text = realloc(new_text, current_new_text_size * sizeof(char));
            checkDynamicAllocation(new_text);
            // }

            // strcat(new_text, replacement);
            memcpy(
                &new_text[new_text_size], replacement, replacement_size + 1);

            new_text_size += replacement_size;
        }

        // text ++;
    }

    for (int i = 0; i < current_new_text_size; i++) {
        printf("%c", new_text[i]);
    }

    free(str_text_char);
    free(new_text);
    free(text);

    return 0;
}
