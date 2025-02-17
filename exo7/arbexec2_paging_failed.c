#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define CHEMIN_MAX 512

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

// Execute la cmd sur chaque chemin trouve
void execChild(char *path, int argC, char *argV[]){
    // for (size_t i = 0; i < files.rSize; i++){
        switch (fork()){
        case -1:
            raler(1, "Zut! Erreur de fork");
            break;
        case 0:
            argV[argC-2] = path;
            execvp(argV[0], argV);
            raler(1, "Erreur d'exec");
            break;
        default:
            // return;
            break;
        }
    // }
    // int raison;

}

// Fonction pour creer des chemin par lecture en bloc et puis lance la cmd
void makePaths(const char *buffer, size_t bufferSize, int argC, char *argV[]){
    char *path;     // C'est un chemin, c'est un char
    path = malloc(CHEMIN_MAX+1);
    int nChilds = 0;
    int pathSize = 0;
    for(size_t i=0; i<bufferSize; i++){
        if (buffer[i] != '\n'){
            // memmove(&path[j], &buffer[i], 1);

            // printf("%c", path[j]);

            pathSize++;
        }else{
            // pushTabext(files, path, j+1);
            CHK_SN(pathSize);
            memmove(&path[0], &buffer[i-pathSize], pathSize);
            path[pathSize] = '\0';
            nChilds ++;
            execChild(path, argC, argV);
            
            // printf("\n");

            pathSize = 0;
        }
        
    }
    free(path);

    int raison;
    for (int i = 0; i < nChilds; i++){
        CHK_PS(wait(&raison));
        if (!(WIFEXITED(raison) && WEXITSTATUS(raison) == 0))
            raler(0, "L'un des processus fils ne s'est pas termine comme prevu");
            // break;
    }
    nChilds = 0;

}



// Main
int main(int argc, char *argv[]){


    // execlp("more", "more", "/usr/include/values.h", NULL);



    int page[2];
    pipe(page);

// -------------------------------------------
    switch (fork()){
    case -1:
        raler(1, "fork failed");
        break;
    case 0:;
        close(page[1]);
        // // dup2(page[1], 1);
        // // printf("MANNNNNNNJARo\n");
        dup2(page[0], 0);
        close(page[0]);
        // // write(f, temp, 1);
        // size_t rd = read(page[0], temp, PIPE_BUF);
        // write(0, temp, PIPE_BUF);
        // // execlp("wc", "wc", NULL);
        // execlp("more", "more", NULL);
        // raler(1, "execlp failed!");
        
        // close(fdW);
        // int fdR = open(fifoPath, O_RDONLY);
        // dup2(fdR, 0);
        // close(fdR);
        // char temp[PIPE_BUF];
        // size_t rd = read(0, temp, 1);
        // write(0, temp, rd);

        // execlp("less", "less", NULL);
        // char temp[PIPE_BUF];
        // while (read(0, temp, PIPE_BUF) > 0);
        // write(page[1], temp, PIPE_BUF);
        // close(page[1]);

        execlp("more", "more", NULL);
        // execlp("tree", "tree", NULL);
        // execlp("wc", "wc", NULL);
        raler(1, "execlp failed!");
    default:
        // read(page[0], temp, 1);
        close(page[0]);
        dup2(page[1], 1);
        close(page[1]);
        execlp("ls", "ls", "-1", "/usr/include", NULL);
        // execlp("cat", "cat", "/usr/include/values.h", NULL);
        // execlp("echo", "echo", "/usr/include", NULL);
        break;
    }
// -------------------------------------------



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
    // char *dir;
    // CHK_MA(dir = malloc(strlen(argv[1])+1));
    // CHK_SN(snprintf(dir, CHEMIN_MAX, "%s", argv[1]));
    // Constitution des arguements
    char *args[argc];
    for (int i = 0; i < argc-2; i++)
        args[i] = argv[2+i];
    args[argc-2] = NULL;      // A modifier plus tard!!!!
    args[argc-1] = NULL;
    
    // printf("%s\n", dir);
    // for (int i = 0; i < argc-2; i++)
    //     printf("%s\n", args[i]);    

    int dirLength = strlen(argv[1]);
    CHK_SN(dirLength);
        

    // Exploration de l'arborescence
    // scanDirectory(sFlag, dir, argc-optind+1, args);
    
    int nFils = 0;
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
        execlp("find", "find", argv[1], "-type", "f", NULL);
        raler(1, "execlp failed!");
    default:;
        // Pagination
        // int page[2];
        // pipe(page);
        // close(page[0]);
        // dup2(page[1], 1);

        // struct tabext files;
        // initTabext(&files);

            // // -------------------------------------------
            //     int page[2];
            //     pipe(page);

            //     switch (fork()){
            //     case -1:
            //         raler(1, "fork failed");
            //         break;
            //     case 0:;
            //         close(page[1]);
            //         // // dup2(page[1], 1);
            //         // // printf("MANNNNNNNJARo\n");
            //         dup2(page[0], 0);
            //         close(page[0]);
            //         // // write(f, temp, 1);
            //         // size_t rd = read(page[0], temp, PIPE_BUF);
            //         // write(0, temp, PIPE_BUF);
            //         // // execlp("wc", "wc", NULL);
            //         // execlp("more", "more", NULL);
            //         // raler(1, "execlp failed!");
                    
            //         // close(fdW);
            //         // int fdR = open(fifoPath, O_RDONLY);
            //         // dup2(fdR, 0);
            //         // close(fdR);

            //         // size_t rd = read(0, temp, PIPE_BUF);
            //         // write(0, temp, rd);

            //         execlp("more", "more", NULL);
            //         raler(1, "execlp failed!");
            //     default:
            //         // read(page[0], temp, 1);
            //         close(page[0]);
            //         dup2(page[1], 1);
            //         close(page[1]);
            //         break;
            //     }
            //     //-----------------------------



        // char *path = malloc(CHEMIN_MAX);
        unsigned char *buffer = malloc(PIPE_BUF);
        int raison;

        close(tube[1]);
        



        size_t bufferSize;
        while ((bufferSize = read(tube[0], buffer, PIPE_BUF)) > 0){
            makePaths(buffer, bufferSize, argc, args);
        }
        



        // size_t bufferSize = read(tube[0], buffer, PIPE_BUF);
        // makePaths(buffer, bufferSize, &files);

        // printf("%d\n", PIPE_BUF);

        // dup2(page[1], 1);
        // nFils++;

        // execCmd(files, argc, args);
        
        //-------------

        // for (int i = 0; i < 1; i++){      // On attend le find ...
            CHK_PS(wait(&raison));
            if (!(WIFEXITED(raison) && WEXITSTATUS(raison) == 0))
                raler(0, "Le processus find ne s'est pas termine comme prevu");
                // break;
        // }

        //-----------------
        


        
        // freeTabext(&files);
        
        close(tube[0]);
        break;
    }


    // free(dir);
    return 0;
}