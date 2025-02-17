#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>

#define CHEMIN_MAX 512
#define TAILLE_BLOC 4096

// Verification des appels de primitives systemes
#define CHECK(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
    } while (0)
// Raler
noreturn void raler(int syserr, const char* fmt, ...);
/* POUR L'EXTRACTION */
// Extrait un repertoire du fichier archive et le rempli
void fillDirectory(int fileDesc, const char* dirPath);
// Lit le contenu idique par bloc de tailles TAILLE_BLOC
void readBuffer(int fileDesc, void* buffer, uint64_t nBytes);
// Cree un fichier regulier a partir de l'archive
void createRegFile(int fileDesc, const char* filePath, mode_t fileRights);
// Cree un lien symbolique
void createSymLink(int fileDesc, const char* linkPath);
// Verifie si on a atteint la fin lecture/ecriture de l'archive
bool atEndOfFIle(int fileDesc);
// Verifie si le chemin 1 est un sous-chemin(fichier/rep/sym) de 2
bool isSubFile(const char* filePath1, const char* filePath2);
/* POUR LA COMPRESSION */
// Ajouter le repertoire et ses objects au fichier archive
void addDirectory(int archiveFileDesc, const char* dirPath,
                  struct stat dirInfo);
// Ecrit dans le fichier archive par bloc
void writeBuffer(int fileDesc, const void* buffer, uint64_t nBytes);
// Permet de vider le buffer d'ecriture vers l'archive a la fin d'un fichier
void emptyBuffer(int fileDesc);
// Ajoute un fichier regulier dans l'archive
void addRegFile(int archiveFileDesc, const char* regFilePath,
                struct stat regFileInfo);
// Ajoute un lien symbolique
void addSymLink(int arcFileDesc, const char* symLinkPath,
                struct stat symLinkInfo);
// Ajoute les info communes a toutes les entrees (type, permissions, etc..)
void addCommonHeader(int arcFileDesc, char fileType, const char* filePath,
                     struct stat fileInfo);

int main(int argc, char* argv[]) {
    switch (argc) {
        case 3:;  // Extraction
            if (strcmp(argv[1], "x") != 0) {
                raler(0, "Mauvaise commance (c'est 'x' pour 'extraction') ");
            } else {
                if (strlen(argv[2]) > CHEMIN_MAX) raler(0, "Chemin trop long");
                int archiveFileDesc = open(argv[2], O_RDONLY, 0666);
                if (archiveFileDesc == -1)
                    raler(1, "%s: %s", "Erreur d'ouverture de", argv[2]);
                uint8_t* csmi = malloc(5);  // Verification du type du fichier
                if (csmi == NULL) raler(1, "Probleme d'allocation");
                readBuffer(archiveFileDesc, csmi, 4);
                csmi[4] = '\0';
                if (strcmp((char*)csmi, "csmi") != 0)
                    raler(0, "Il ne s'agit pas d'une bonne archive '.arc'");
                const char* baseDirPath =
                    "";  // Ca pourrait etre le nom du dossier dans lequel on
                         // veut decompresser
                fillDirectory(archiveFileDesc, baseDirPath);
                CHECK(close(archiveFileDesc));
                free(csmi);
            }
            break;
        case 4:  // Compression
            if (strcmp(argv[1], "c") != 0) {
                raler(0, "Mauvaise commance (c'est 'c' pour 'compression')");
            } else {
                if (strlen(argv[3]) > CHEMIN_MAX ||
                    strlen(argv[2]) > CHEMIN_MAX)
                    raler(0, "Chemin trop long");
                int arcFileDesc =
                    open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (arcFileDesc == -1)
                    raler(1, "%s: %s", "Erreur d'ouverture de", argv[2]);
                uint8_t csmi[4] = {0x63, 0x73, 0x6d, 0x69};
                writeBuffer(arcFileDesc, csmi, 4);
                struct stat dirInfo;
                CHECK(stat(argv[3], &dirInfo));
                addDirectory(arcFileDesc, argv[3], dirInfo);
                emptyBuffer(arcFileDesc);  // Vidons le buffer
                CHECK(close(arcFileDesc));
            }
            break;
        default:
            raler(0, "Nombre d'arguments incorrect");
            break;
    }
    return 0;
}

void readBuffer(int fileDesc, void* buffer, uint64_t nBytes) {
    static uint8_t* _localBuffer;         // Le buffer local
    static uint32_t _remainingBytes = 0;  // Espace restant dans le buffer
    static uint64_t globalBufferPos =
        0;                        // Position actuelle dans le buffer a remplir
    static int previousFile = 0;  // Precedent fichier lu
    static ssize_t readSize;      // Quantite d'octets recopiee
    if (fileDesc != previousFile)
        _remainingBytes = 0;  // Pour forcer le vidange du buffer
    if (_remainingBytes <= 0) {
        if (_localBuffer != NULL) {
            free(_localBuffer);
            _localBuffer = NULL;
        }
        _localBuffer = malloc(TAILLE_BLOC);
        if (_localBuffer == NULL) raler(1, "Problemes d'allocation");
        readSize = read(fileDesc, _localBuffer, TAILLE_BLOC);
        if (readSize == 0)
            return;  // Fin du fichier
        else if (readSize <= -1)
            raler(1, "Erreur de lecture");
        else {
            CHECK(lseek(fileDesc, -readSize, SEEK_CUR));
            _remainingBytes = readSize;
        }
    }
    if (nBytes <= _remainingBytes) {
        memmove(buffer + globalBufferPos,
                &_localBuffer[readSize - _remainingBytes], nBytes);
        _remainingBytes -= nBytes;
        CHECK(lseek(fileDesc, nBytes, SEEK_CUR));
        globalBufferPos = 0;  // Necessaire pour les prochains appels
        previousFile = fileDesc;
    } else {  // nBytes > _remainingBytes
        memmove(buffer + globalBufferPos,
                &_localBuffer[readSize - _remainingBytes], _remainingBytes);
        uint64_t copiedBytes = _remainingBytes;
        _remainingBytes = 0;
        CHECK(lseek(fileDesc, copiedBytes, SEEK_CUR));
        globalBufferPos += copiedBytes;
        readBuffer(fileDesc, buffer, nBytes - copiedBytes);
    }
}

void fillDirectory(int fileDesc, const char* dirPath) {
    uint8_t nextEntryType;
    uint32_t nextEntryRights;
    uint16_t nextEntryPathLength;
    uint8_t* nextEntryPath = NULL;
    do {
        if (atEndOfFIle(fileDesc) == true) {
            if (nextEntryPath != NULL) free(nextEntryPath);
            return;
        } else {
            readBuffer(fileDesc, &nextEntryType, 1);
            readBuffer(fileDesc, &nextEntryRights, 2);
            nextEntryRights &= 0777;
            readBuffer(fileDesc, &nextEntryPathLength, 2);
            if (nextEntryPath != NULL) free(nextEntryPath);
            nextEntryPath = malloc(nextEntryPathLength + 1);
            if (nextEntryPath == NULL) raler(1, "Erreur d'allocation");
            readBuffer(fileDesc, nextEntryPath, nextEntryPathLength);
            nextEntryPath[nextEntryPathLength] = '\0';
            if (strlen((char*)nextEntryPath) > CHEMIN_MAX)
                raler(0, "%s: %s", "Chemin trop long", nextEntryPath);
            switch (nextEntryType) {
                case 0x01:  // Fichier regulier
                    createRegFile(fileDesc, (char*)nextEntryPath,
                                  nextEntryRights);
                    break;
                case 0x02:  // Repertoire
                    CHECK(mkdir((char*)nextEntryPath, (mode_t)nextEntryRights));
                    fillDirectory(fileDesc, (char*)nextEntryPath);
                    break;
                case 0x03:  // Lien symbolique
                    createSymLink(fileDesc, (char*)nextEntryPath);
                    break;
                default:
                    raler(0, "Mauvais type de fichier");
                    break;
            }
        }
    } while (isSubFile((char*)nextEntryPath, dirPath) == true);
    if (nextEntryPath != NULL) free(nextEntryPath);
}

void createRegFile(int fileDesc, const char* filePath, mode_t fileRights) {
    uint32_t dateModified;
    readBuffer(fileDesc, &dateModified, 4);
    uint64_t fileSize;
    readBuffer(fileDesc, &fileSize, 8);
    uint8_t* buffer = malloc(fileSize);
    if (buffer == NULL) raler(1, "Probleme d'allocation");
    readBuffer(fileDesc, buffer, fileSize);
    int regFileDesc = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    writeBuffer(regFileDesc, buffer, fileSize);
    emptyBuffer(regFileDesc);
    close(regFileDesc);
    free(buffer);
    struct utimbuf fileDateAttr;
    fileDateAttr.actime = time(NULL);  // On suppose ...
    fileDateAttr.modtime = (time_t)dateModified;
    CHECK(utime(filePath, &fileDateAttr));
    CHECK(chmod(filePath, fileRights));
}

void createSymLink(int fileDesc, const char* linkPath) {
    uint16_t targetFilePathSize = 0;
    readBuffer(fileDesc, &targetFilePathSize, 2);
    uint8_t* targetFilePath = malloc(targetFilePathSize + 1);
    if (targetFilePath == NULL) raler(1, "Probleme d'allocation");
    readBuffer(fileDesc, targetFilePath, targetFilePathSize);
    targetFilePath[targetFilePathSize] = '\0';
    if (strlen((char*)targetFilePath) > CHEMIN_MAX)
        raler(0, "La cible du lien est trop longue");
    CHECK(symlink((char*)targetFilePath, linkPath));
    free(targetFilePath);
}

bool atEndOfFIle(int fileDesc) {
    uint64_t currentPosition = lseek(fileDesc, 0, SEEK_CUR);
    uint64_t fileSize = lseek(fileDesc, 0, SEEK_END);
    lseek(fileDesc, currentPosition, SEEK_SET);
    if (currentPosition >= fileSize)
        return true;
    else
        return false;
}

bool isSubFile(const char* filePath1, const char* filePath2) {
    if (strcmp(filePath2, "") == 0)  // On se trouve a la racine
        return true;
    else {
        int filePath2Length = strlen(filePath2);
        if (strncmp(filePath1, filePath2, filePath2Length) == 0 &&
            filePath1[filePath2Length] == '/')
            return true;
        else
            return false;
    }
}

void addDirectory(int archiveFileDesc, const char* dirPath,
                  struct stat dirInfo) {
    addCommonHeader(archiveFileDesc, 0x02, dirPath, dirInfo);
    DIR* dir;
    dir = opendir(dirPath);
    if (dir == NULL)
        raler(1, "%s: %s", "Probleme d'ouverture du repertoire", dirPath);
    struct dirent* dirEntry;
    char* dirEntryPath = malloc(CHEMIN_MAX);
    if (dirEntryPath == NULL)
        raler(1, "Probleme pour contenir un nouveau chemin de fichier");
    int dirEntryPathLength;
    struct stat entryStats;
    while ((dirEntry = readdir(dir)) != NULL) {
        if (strcmp(dirEntry->d_name, ".") != 0 &&
            strcmp(dirEntry->d_name, "..") != 0) {
            dirEntryPathLength = snprintf(dirEntryPath, CHEMIN_MAX, "%s/%s",
                                          dirPath, dirEntry->d_name);
            if (dirEntryPathLength >= CHEMIN_MAX)
                raler(0, "%s: %s\n", "Chemin d'acces trop long", dirEntryPath);
            CHECK(lstat(dirEntryPath, &entryStats));
            switch (entryStats.st_mode & S_IFMT) {
                case S_IFREG:
                    addRegFile(archiveFileDesc, dirEntryPath, entryStats);
                    break;
                case S_IFDIR:
                    addDirectory(archiveFileDesc, dirEntryPath, entryStats);
                    break;
                case S_IFLNK:
                    addSymLink(archiveFileDesc, dirEntryPath, entryStats);
                    break;
                default:
                    break;  // On ne traite pas ces cas
            }
        }
    }
    CHECK(closedir(dir));
    free(dirEntryPath);
}

void writeBuffer(int fileDesc, const void* buffer, uint64_t nBytes) {
    static uint8_t* _localBuffer;         // Les donnees lues de fileDesc
    static uint32_t _remainingBytes = 0;  // Place restante
    static uint64_t globalBufferPos =
        0;  // Quantite totale ecrite (pour des blocs de taille > TAILLE_BLOC)
    static int previousFile = 0;
    if (fileDesc != previousFile) {  // Si on ne copie pas vers le meme fichier,
                                     // tout reinitialiser
        _localBuffer = NULL;
        _remainingBytes = 0;  // Pour forcer le vidange du buffer
    }
    if (_localBuffer != NULL && (_remainingBytes <= 0 || nBytes == 0)) {
        int writtenSize =
            write(fileDesc, _localBuffer, TAILLE_BLOC - _remainingBytes);
        if (writtenSize == 0)
            return;  // _localBuffer vide? Disque plein?
        else if (writtenSize <= -1)
            raler(1, "Erreur d'ecriture depuis le buffer");
        else {
            free(_localBuffer);
            _localBuffer = NULL;
        }
        if (nBytes == 0) {  // Indique qu'on a fini avec ce fichier
            previousFile = 0;
            return;
        }
    }
    if (_remainingBytes <= 0) {
        _localBuffer = malloc(TAILLE_BLOC);
        if (_localBuffer == NULL) raler(1, "Probleme d'allocation");
        _remainingBytes = TAILLE_BLOC;
    }
    if (nBytes <= _remainingBytes) {  // On ajoute nBytes au buffer
        memmove(&_localBuffer[TAILLE_BLOC - _remainingBytes],
                buffer + globalBufferPos, nBytes);
        _remainingBytes -= nBytes;
        globalBufferPos = 0;  // Pour les prochains appels
        previousFile = fileDesc;
    } else {  // On ajoute ce qu'on peut au buffer
        memmove(&_localBuffer[TAILLE_BLOC - _remainingBytes],
                buffer + globalBufferPos, _remainingBytes);

        uint64_t copiedBytes = _remainingBytes;
        globalBufferPos += copiedBytes;
        _remainingBytes = 0;
        previousFile = fileDesc;
        writeBuffer(fileDesc, buffer, nBytes - copiedBytes);
    }
}

void emptyBuffer(int fileDesc) { writeBuffer(fileDesc, "", 0); }

void addRegFile(int arcFileDesc, const char* regFilePath,
                struct stat regFileInfo) {
    addCommonHeader(arcFileDesc, 0x01, regFilePath, regFileInfo);
    uint32_t lastModDate = regFileInfo.st_mtime;
    writeBuffer(arcFileDesc, &lastModDate, 4);
    uint64_t regFileSize = regFileInfo.st_size;
    writeBuffer(arcFileDesc, &regFileSize, 8);
    int regFile = open(regFilePath, O_RDONLY);
    if (regFile == -1) raler(1, "%s: %s", "Impossible d'ouvrir", regFilePath);
    uint8_t* buffer = malloc(regFileSize);
    if (buffer == NULL) raler(1, "Erreur d'allocation");
    readBuffer(regFile, buffer, regFileSize);
    writeBuffer(arcFileDesc, buffer, regFileSize);
    CHECK(close(regFile));
    free(buffer);
}

void addSymLink(int arcFileDesc, const char* symLinkPath,
                struct stat symLinkInfo) {
    addCommonHeader(arcFileDesc, 0x03, symLinkPath, symLinkInfo);
    uint8_t* targetPath = malloc(CHEMIN_MAX);
    if (targetPath == NULL) raler(1, "Erreur d'allocation");
    int targetPathSize =
        readlink(symLinkPath, (char*)targetPath, CHEMIN_MAX + 1);
    if (targetPathSize == -1 || targetPathSize > CHEMIN_MAX)
        raler(1, "Erreur de lecture de la cible du lien ou cible trop longue");
    writeBuffer(arcFileDesc, &targetPathSize, 2);
    writeBuffer(arcFileDesc, targetPath, targetPathSize);
    free(targetPath);
}

void addCommonHeader(int arcFileDesc, char fileType, const char* filePath,
                     struct stat fileInfo) {
    writeBuffer(arcFileDesc, &fileType, 1);
    uint16_t fileRights = fileInfo.st_mode & 0777;
    writeBuffer(arcFileDesc, &fileRights, 2);
    uint16_t pathLength = strlen(filePath);
    writeBuffer(arcFileDesc, &pathLength, 2);
    writeBuffer(arcFileDesc, filePath, pathLength);
}

noreturn void raler(int syserr, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr) perror("");
    exit(1);
}