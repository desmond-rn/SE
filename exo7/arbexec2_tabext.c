#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <getopt.h>
#include <dirent.h>
#define CHEMIN_MAX 512
#define INCREMENT 64
// Verifie les appels aux primitives systemes
#define CHK_PS(v)                     \
    do {                             \
        if ((v) == -1) raler(1, #v); \
    } while (0)
// Verifie les appels a malloc et realloc
#define CHK_MA(v)                     \
    do {                             \
        if ((v) == NULL) raler(1, #v); \
    } while (0)
// Verifie les appels a snprintf
#define CHK_SN(v)                     \
    do {                             \
        if (((v) > CHEMIN_MAX) || ((v) <= 0)) raler(0, "%s\n%s", #v ,"Oops! Chemin trop long, ou autre erreur..."); \
    } while (0)
// Rale
noreturn void raler(int syserr, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    if (syserr) perror("");
    exit(1);
}
// Tableau extensible contenant une liste de chaines de caracteres
struct tabext {
    size_t rSize, eSize;  // real size and effective size
    char **list;
};
// Initialise le tableau extensible
void initTabext(struct tabext *extArray){
    extArray->rSize = extArray->eSize = 0;
    CHK_MA(extArray->list = malloc(INCREMENT*sizeof(char*)));
    extArray->eSize = INCREMENT;
}
// Realloue le tableau extensible
void reallocTabext(struct tabext *extArray){
    CHK_MA(extArray->list = realloc(extArray->list, (extArray->eSize+INCREMENT)*sizeof(char*)));
    extArray->eSize += INCREMENT;
}

// Ajoute un element au tableau extensible
void pushTabext(struct tabext *extArray, const char *elem, int elemLength){
    if (extArray->rSize == extArray->eSize)
        reallocTabext(extArray);
    CHK_MA(extArray->list[extArray->rSize] = malloc(elemLength+1));
    CHK_SN(snprintf(extArray->list[extArray->rSize], elemLength, "%s", elem));
    extArray->list[extArray->rSize][elemLength] = '\0';
    extArray->rSize ++;
}

// Desalloue le tableau extensible
void freeTabext(struct tabext *extArray){
    for (size_t i = 0; i < extArray->rSize; i++)
        free(extArray->list[i]);
    free(extArray->list);
    // extArray->list = NULL;
    extArray->eSize = extArray->rSize = 0;
}

// Fonction pour lire des blocs du tube
void makePaths(unsigned char *buffer, size_t bufferSize, struct tabext *files){
    char *path;     // C'est un chemin, c'est un char
    path = malloc(CHEMIN_MAX);
    int j = 0;
    for(size_t i=0; i<bufferSize; i++){
        if (buffer[i] != '\n'){
            memmove(&path[j], &buffer[i], 1);

            // printf("%c", path[j]);

            j++;
        }else{
            pushTabext(files, path, j+1);
            
            // printf("\n");

            j = 0;
        }
        
    }

}

void execCmd(struct tabext files, int argC, char *argV[]){
    for (size_t i = 0; i < files.rSize; i++){
        switch (fork()){
        case -1:
            raler(1, "Zut! Erreur de fork");
            break;
        case 0:
            argV[argC-2] = files.list[i];
            execvp(argV[0], argV);
            raler(1, "Erreur d'exec");
            break;
        default:
            break;
        }
    }
    int raison;
    for (size_t i = 0; i < files.rSize; i++){
        CHK_PS(wait(&raison));
        if ((WIFEXITED(raison) && WEXITSTATUS(raison) != 0))
            exit(2);
        else if (WIFSIGNALED(raison) || WIFSTOPPED(raison))
            raler(0, "L'un des processus ne s'est pas terminé comme prévu");
    }
}

// Main
int main(int argc, char *argv[]){

    // int page[2];
    // pipe(page);
    // // // close(page[0]);
    // // // dup2(page[1], 2);
    // if (dup2(page[1], 1) == -1)
    //     raler(1, "Cannot redirect stdout");
    // // // close(page[1]);

    // // // fprintf(stderr, "MANNNNNNNJARo\n");
    // // // fprintf(stdout, "MANNNNNNNJARo\n");
    // write(1, "MANNNNNNNJARo\n", 10);

    char temp[PIPE_BUF];
    // int rd = read(page[0], temp, 10);
    // // read(page[0], temp, 10);
    // temp[10] = '\0';
    // // temp[0] = 'C';
    // fprintf(stderr, "%s\n", temp);
    // int f = open("log", O_WRONLY|O_TRUNC|O_CREAT, 0666);
    // write(f, temp, 10);
    // // // write(f, "%s\n", temp);

    // // exit(2);




    // ---------------

    // char * fifoPath = "./page";
    // mkfifo(fifoPath, 0666);
    // int fdW = open(fifoPath, O_WRONLY);
    // dup2(fdW, 1);
    // close(fdW);

    // ----------------




    if (argc < 3)
        raler(0, "Nombre d'arguments insuffisant");
    // Constitution du repertoire racine
    char *dir;
    CHK_MA(dir = malloc(strlen(argv[1])+1));
    CHK_SN(snprintf(dir, CHEMIN_MAX, "%s", argv[1]));
    // Constitution des arguements
    char *args[argc];
    for (int i = 0; i < argc-2; i++)
        args[i] = argv[2+i];
    args[argc-2] = NULL;      // A modifier plus tard!!!!
    args[argc-1] = NULL;
    
    printf("%s\n", dir);
    for (int i = 0; i < argc-2; i++)
        printf("%s\n", args[i]);    
        

    // Exploration de l'arborescence
    // scanDirectory(sFlag, dir, argc-optind+1, args);
    
    int tube[2];
    CHK_PS(pipe(tube));


    switch (fork()){
    case -1:
        raler(1, "Fork failed!");
        break;
    case 0:
        // close(page[0]);
        CHK_PS(close(tube[0]));
        CHK_PS(dup2(tube[1], 1));
        CHK_PS(close(tube[1]));
        execlp("find", "find", dir, "-type", "f", NULL);
        raler(1, "execlp failed!");
    default:;
        // Pagination
        // int page[2];
        // pipe(page);
        // close(page[0]);
        // dup2(page[1], 1);

        struct tabext files;
        initTabext(&files);
        
        char *path = malloc(CHEMIN_MAX);
        unsigned char buffer[PIPE_BUF];
        int raison;

        close(tube[1]);
        
        CHK_PS(wait(&raison));
        if ((WIFEXITED(raison) && WEXITSTATUS(raison) != 0))
            exit(3);
        else if (WIFSIGNALED(raison) || WIFSTOPPED(raison))
            raler(0, "L'un des processus ne s'est pas terminé comme prévu");

        size_t bufferSize = read(tube[0], buffer, PIPE_BUF);
        makePaths(buffer, bufferSize, &files);

        // printf("%d\n", PIPE_BUF);

        // dup2(page[1], 1);

        execCmd(files, argc, args);
        
        freeTabext(&files);
        
        close(tube[0]);
        break;
    }


    // ----------------

    // switch (fork()){
    // case -1:
    //     raler(1, "fork failed");
    //     break;
    // case 0:;
    //     // close(page[1]);
    //     // // dup2(page[1], 1);
    //     // // printf("MANNNNNNNJARo\n");
    //     // // dup2(page[0], 0);
    //     // // close(page[0]);
    //     // // write(f, temp, 1);
    //     // size_t rd = read(page[0], temp, PIPE_BUF);
    //     // write(0, temp, PIPE_BUF);
    //     // // execlp("wc", "wc", NULL);
    //     // execlp("more", "more", NULL);
    //     // raler(1, "execlp failed!");
        
    //     close(fdW);
    //     int fdR = open(fifoPath, O_RDONLY);
    //     dup2(fdR, 0);
    //     close(fdR);

    //     // size_t rd = read(0, temp, PIPE_BUF);
    //     // write(0, temp, rd);

    //     execlp("more", "more", NULL);
    // default:
    //     // read(page[0], temp, 1);
    //     // close(page[0]);
    //     // dup2(tube[1], 1);
    //     // close(page[1]);
    //     break;
    // }


    // ----------------

    free(dir);
    return 0;
}