#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h> //memset
#include <signal.h>
#include <poll.h>
#include "obslugaPliku.h"

int checkPath(char *pathToFile);
int countFifos(char **allFiles, int numberOfLines);
int *openFifos(int numberOfFifos, char **fifos);
int oplacone(int fifoFd);
static void rtsig_handler(int sig, siginfo_t *si, void *ucontext);
static void usrsig_handler();
void selectFifos(char **fifos, char **allFiles, int numberOfLines);
void printHelp();

int iloscWydanych = 0;

struct towar
{
    pid_t PID;
    int sigNum;
    short idTowaru;
};

struct status
{
    int fd;
    int status;
};

struct status *czyPisanoDoTegoFd;
struct status *wyslane;
int *doTychPisalem;

int main(int argc, char* argv[])
{
    int opt = 0;
    int sigNum = 0;
    int fd = 0;
    int numberOfLines = 0;
    char *pathToFile;
    struct towar paczka;
    memset(&paczka, 0, sizeof(paczka));

    if(argc == 4 && argv[1][1] == 's')
    {
        pathToFile = argv[3];
    }
    else if(argc == 4 && argv[1][1] != 's')
    {
        pathToFile = argv[1];
    }
    else
    {
        printf("Zla liczba parametrow.\n");
        printHelp();
        exit(EXIT_FAILURE);
    }

    while( (opt = getopt(argc, argv, "s:")) != -1)
    {
        switch (opt)
        {
        case 's':
            sigNum = strtol(optarg, NULL, 10);
            if(sigNum < 34 || sigNum > 64)
            {
                printf("Zly numer sygnalu. \"kill -l | grep SIGRT\" - aby sprawdzic mozliwe numery.\n");
                exit(EXIT_FAILURE);
            }
            break;
        
        default:
            printHelp();
            break;
        }
    }
    paczka.PID = getpid();
    paczka.sigNum = sigNum;
    paczka.idTowaru = iloscWydanych;

    fd = openFile(pathToFile);
    numberOfLines = getNumberOfFiles(fd);
    char *allFiles[numberOfLines];
    getAllFiles(fd, allFiles);

    struct status zapisane;
    memset(&zapisane, 0, sizeof(zapisane));
    struct status czyPisano;
    memset(&czyPisano, 0, sizeof(czyPisano));

    struct sigaction rtsa;
    sigemptyset(&rtsa.sa_mask);
    rtsa.sa_sigaction = rtsig_handler;
    rtsa.sa_flags = SA_SIGINFO;
    sigaction(sigNum, &rtsa, NULL);

    struct sigaction usrsa;
    memset(&usrsa, '\0', sizeof(usrsa));
    usrsa.sa_sigaction = &usrsig_handler;
    usrsa.sa_flags = 0;
    sigaction(SIGUSR1, &usrsa, NULL);
    
    int stat = 0;
    int x = 0;
    while(1)
    {
        int nIt = 0;
        int nSize = 0;
        int numBytes = 0;
        int los = 0;
        int n = countFifos(allFiles, numberOfLines);
        char *fifos[n];
        int *fifoFds;
        selectFifos(fifos, allFiles, numberOfLines);

        for(int i = 0; i<n; i++)
        {
            int tempFd = 0;
            tempFd = open(fifos[i], O_WRONLY | O_NONBLOCK);
            if(tempFd < 0)
            {
                srand(time(NULL));
                los = ((rand()%2) == 1) ? 1 : 0;
                if(los == 1)
                {
                    printf("LOS: %d\n", los);
                    unlink(fifos[i]);
                }
            }
            else if(tempFd > 0)
            {
                nSize++;
                fifoFds = (int*)realloc(fifoFds,nSize*sizeof(int));
                fifoFds[nIt] = tempFd;
                nIt++;
            }  
        }

        for(int i = 0; i<nSize; i++)
        {
            numBytes = 0;
            if( stat == 0 || oplacone(fifoFds[i]) == 1 )
            {
                numBytes = write(fifoFds[i], &paczka, sizeof(paczka));
            }
            czyPisano.fd = fifoFds[i];
            czyPisano.status = 1;
            czyPisanoDoTegoFd = (struct status*)realloc(czyPisanoDoTegoFd, (i+1)*sizeof(*czyPisanoDoTegoFd));

            if(numBytes == sizeof(paczka))
            {
                if( oplacone(fifoFds[i]) == 1 )
                    printf("zaplacono\n");
                
                doTychPisalem = (int*)realloc(doTychPisalem, (i+1)*sizeof(int));
                doTychPisalem[i] = fifoFds[i];
                zapisane.fd = fifoFds[i];
                zapisane.status = 0;
                wyslane = (struct status*)realloc(wyslane, (iloscWydanych+1)*sizeof(*wyslane));
                wyslane[iloscWydanych] = zapisane;
                printf("id: %d, fd: %d, status: %d\n", iloscWydanych, wyslane[iloscWydanych].fd, wyslane[iloscWydanych].status);

                iloscWydanych++;
                paczka.idTowaru = iloscWydanych;
                close(fifoFds[i]);
            }
        }
        x++;
        if( 3 == x)
            stat = 1;
    }
    close(fd);
    return 0;
}

void rtsig_handler(int sig, siginfo_t *si, void *ucontext)
{
   // printf("Otrzymano zaplate za towar: %d\n", si->si_value.sival_int);
    for(int i = 0; i<iloscWydanych; i++)
    {
        if(si->si_value.sival_int == i)
        {
           // printf("i: %d\n", wyslane[i].fd);
            wyslane[i].status = 1;
        }
    }
}

void usrsig_handler()
{
    printf("Przechwycono SIGUSR1!\n");
}

int checkPath(char *pathToFile)
{
    struct stat st;
    if( !(stat(pathToFile, &st)) && S_ISFIFO(st.st_mode) )
    {
        return 1;
    }

    return 0;
}

int countFifos(char **allFiles, int numberOfLines)
{
    int exists = 0;
    int n = 0;
    for(int i = 0; i<numberOfLines; i++)
    {
        exists = checkPath(allFiles[i]);
        if(exists)
        {
            n++; 
        }
    }
    return n;
}

void selectFifos(char **fifos, char **allFiles, int numberOfLines)
{
    int n = 0;
    int exists = 0;
    for(int i = 0; i<numberOfLines; i++)
    {
        exists = checkPath(allFiles[i]);
        if(exists)
        {
            fifos[n] = allFiles[i];
            n++; 
        }
    }
}

int oplacone(int fifoFd)
{
    for(int i = 0; i<iloscWydanych; i++)
    {
        if(wyslane[i].fd == fifoFd && wyslane[i].status == 1)
        {
            return 1;
        }
    }

    return 0;
}

void printHelp()
{
    printf("Przyklady uzycia:\n");
    printf("\t ./<sprzedawca> -s <zaplata[34-64]> <plik_konfiguracyjny>\n");
    printf("\t ./<sprzedawca> <plik_konfiguracyjny> -s <zaplata[34-64]>\n");
}
