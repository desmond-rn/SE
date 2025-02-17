#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <dirent.h>
#include <sys/stat.h>

#define CHEMIN_MAX 512
#define INITIAL_LIST_LENGTH 2048

// Strucutre contenant le chemain d'acces et la taille d'un fichier regulier
struct File {
    char path[CHEMIN_MAX];
    size_t size;
};

// Fonction neccessaire a qsort pour la comparaison
int compareFiles(const void* file1, const void* file2);

// Pour afficher les erreurs vraiment critiques
void printErrorDescriptionThenExit(const char* message)
{
    perror(message);
    exit(1);
}

// Pour afficher les erreur critiques mais qui ne modifient pas 'errno'
void printErrorMessageThenExit(const char* message)
{
    fprintf(stderr, "%s", message);
    exit(2);
}

/**
 * @brief  Fonction recurcive pour explorer l'arborescence
 * @note   Afin de permettre la continuation de l'exploration
 *          en cas d'erreur mineure, cette fonction retourne des -1 pour pour
 *          quitter le repertoire courant "inexplorable"
 * @param  *dirPath: Le chemin d'acces du repertoire en cours d'eploration
 * @param  **fileList: La liste des fichiers a affhicher sur la sortie
 * standar. Il s'agit d'un pointeur sur un pointeur afin qu'il soit modifiable
 * @retval _fileListLength: Une variable statique qui contabilise le
 *          nombre total d'entrees dans *fileList
 */
ssize_t scanDirectory(const char* dirPath, struct File** fileList);

int main(int argc, char* argv[])
{

    switch (argc) {
    case 2:; // Le ';' c'est pour eviter la declaration immediatement apres

        struct File* fileList
            = malloc(INITIAL_LIST_LENGTH * sizeof(struct File));
        if (fileList == NULL)
            printErrorDescriptionThenExit(
                "Probleme lors de l'allocation des premiers 1 Mo pour le "
                "tableau\nError");

        const char* baseDirPath = argv[1];
        ssize_t totalDirLength = scanDirectory(baseDirPath, &fileList);
        if (totalDirLength == -1)
            printErrorMessageThenExit("Aucun fichier n'a pu etre exploré\n");

        // Aucun type de retour
        qsort(fileList, totalDirLength, sizeof(struct File), compareFiles);

        for (ssize_t i = 0; i < totalDirLength; i++)
            printf("%zu\t%s\n", fileList[i].size, fileList[i].path);

        free(fileList);
        break;

    default:;

        fprintf(stderr, "Erreur: Nombre d'arguments incorrect\n");
        break;
    }

    return 0;
}

int compareFiles(const void* file1, const void* file2)
{
    const struct File* file1Prime = file1;
    const struct File* file2Prime = file2;
    if (file1Prime->size > file2Prime->size)
        return 1;
    else if (file1Prime->size == file2Prime->size)
        return 0;
    else
        return -1;
}

ssize_t scanDirectory(const char* dirPath, struct File** fileList)
{

    static size_t _fileListLength = 0; // La taille reelle de la liste
    static size_t _currentMaxListLength = INITIAL_LIST_LENGTH; // Max taille

    DIR* dir;
    dir = opendir(dirPath);
    if (dir == NULL) {
        perror("Error");
        fprintf(stderr, "%s: %s\n", "Probleme d'ouverture du repertoire",
            dirPath);
        return -1; // C'est pas grave, on n'exit pas le programme
    }
    struct dirent* dirEntry;
    char* dirEntryPath = malloc(CHEMIN_MAX);
    if (dirEntryPath == NULL) {
        perror("Error");
        fprintf(stderr, "%s\n",
            "Probleme de memoire pour contenir un nouveau chemin de fichier");
        return -1;
    }
    int dirEntryPathLength;
    struct stat entryStats;

    while ((dirEntry = readdir(dir)) != NULL) {
        if (_fileListLength == _currentMaxListLength) { // A cours de memoire
            struct File* tempFileList;
            tempFileList = realloc(*fileList,
                (_currentMaxListLength + INITIAL_LIST_LENGTH)
                    * sizeof(struct File));
            if (tempFileList == NULL) {
                perror("Error");
                fprintf(stderr, "%s",
                    "Pas assez de memoire pour continuer a explorer\n");
                return _fileListLength;   // Continuons neanmoins l'exploration
            }else {     // Realloc reussi
                *fileList = tempFileList;
                _currentMaxListLength += INITIAL_LIST_LENGTH;
            }
        }

        dirEntryPathLength = snprintf(dirEntryPath, CHEMIN_MAX, "%s%s%s",
            dirPath, "/", dirEntry->d_name);
        if (dirEntryPathLength >= CHEMIN_MAX) {
            fprintf(
                stderr, "%s: %s\n", "Chemin d'acces trop long", dirEntryPath);
            continue;
        }

        if (stat(dirEntryPath, &entryStats) == -1) {
            perror("Error");
            fprintf(stderr, "%s: %s\n",
                "Impossible de recuperer les attributs du fichier",
                dirEntryPath);
            continue; // return -1; // On devrait sortir de ce repertoire  
                       // car il est fort probable que les autres fichiers 
                       // soit aussi illisibles
        }

        if ((entryStats.st_mode & __S_IFMT) == __S_IFREG) {
            if (strncpy((*fileList + _fileListLength)->path, dirEntryPath,
                    dirEntryPathLength)
                == NULL) {
                perror("Error");
                fprintf(stderr, "%s: %s\n",
                    "Impossible de recopier le nom de ce fichier",
                    dirEntryPath);
                continue;
            }
            (*fileList + _fileListLength)->size = entryStats.st_size;
            _fileListLength++;

        } else if ((entryStats.st_mode & __S_IFMT) == __S_IFDIR
            && strcmp(dirEntry->d_name, ".") != 0
            && strcmp(dirEntry->d_name, "..") != 0) {
            if (access(dirEntryPath, F_OK | R_OK) == 0)
                scanDirectory(dirEntryPath,
                    fileList); // La valeur retournée a ce niveau importe peu
            else {
                perror("Error");
                fprintf(stderr, "%s: %s\n",
                    "Problemes de droits de lecture sur", dirEntryPath);
                continue;  // return -1; // Les dossiers voisins sont pareils
            }
        }
    }
    if (closedir(dir) == -1)
        perror("Error");
    free(dirEntryPath);
    return _fileListLength;
}