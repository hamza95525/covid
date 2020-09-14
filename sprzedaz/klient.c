#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include "obslugaPliku.h"

void printHelp();
void openOrCreatePipe(char *pathToFifo, int times);

struct towar
{
    pid_t PID;
    int sigNum;
    short idTowaru;
};

int main(int argc, char* argv[])
{
    int opt = 0;
    int fd = 0;
    int times = 1;
    int numberOfLines = 0;
    char *pathToFile;
    char *randomFile;

    if(argc == 2)
    {
        pathToFile = argv[1];
    }
    else if(argc == 4 && argv[1][1] == 'k')
    {
        pathToFile = argv[3];
    }
    else if(argc == 4 && argv[1][1] != 'k')
    {
        pathToFile = argv[1];
    }
    else
    {
        printf("Zla liczba parametrow.\n");
        printHelp();
        exit(EXIT_FAILURE);
    }

    while( (opt = getopt(argc, argv, "k:")) != -1)
    {
        switch (opt)
        {
        case 'k':
            times = strtol(optarg, NULL, 10);
            break;

        default:
            printHelp();
            exit(EXIT_FAILURE);
        }
    }

    fd = openFile(pathToFile);
    numberOfLines = getNumberOfFiles(fd);
    char *allFiles[numberOfLines];
    getAllFiles(fd, allFiles);

    randomFile = getRandomFile(allFiles, numberOfLines);
    printf("%s times: %d\n", randomFile, times);

    openOrCreatePipe(randomFile, times);
    close(fd);
    return 0;
}

void printHelp()
{
    printf("Przyklady uzycia:\n");
    printf("\t ./<klient> <plik_konfiguracyjny>\n");
    printf("\t ./<klient> -k <liczba_towarow> <plik_konfiguracyjny>\n");
    printf("\t ./<klient> <plik_konfiguracyjny> -k <liczba_towarow>\n");
}

void openOrCreatePipe(char *pathToFifo, int times)
{
    struct towar *odebranaPaczka = malloc(sizeof(struct towar));
    union sigval sv;

    int fd_read = 0;
    umask(0);
    fd_read = mknod(pathToFifo, S_IFIFO|0666, 0);
    if(fd_read < 0 && errno != 17) //17 - EEXIST
    {
        perror("Creating fifo failed");
        exit(EXIT_FAILURE);
    }

    fd_read = open(pathToFifo, O_RDONLY);
    if(fd_read < 0)
    {
        perror("Opening fifo failed");
        exit(EXIT_FAILURE);
    }

    int i = 0;
    int numBytes = 0;
    for(i=0; i<times; i++)
    {   
        numBytes = 0;
        while(!numBytes)
        {
            numBytes = read(fd_read, odebranaPaczka, sizeof(*odebranaPaczka));
        }

        if(numBytes > 0)
        {
            if(odebranaPaczka == NULL)
            {
                printf("Read data is null");
            }
            else
            {
                printf("Read from fifo: size->%d\t%d\t%d\t%d\n", numBytes, odebranaPaczka->PID, odebranaPaczka->sigNum, odebranaPaczka->idTowaru);
                sv.sival_int = odebranaPaczka->idTowaru;
                sigqueue(odebranaPaczka->PID, odebranaPaczka->sigNum, sv);
                //kill(odebranaPaczka->PID, odebranaPaczka->sigNum);
            }
        }
        else
        {
            printf("Nothing was read from FIFO\n");
        }        
    }

    free(odebranaPaczka);
    close(fd_read);
}