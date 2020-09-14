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
int isDone(char **fifos, int n);
int oplacone(int fifoFd);
char *customSprintf(long i, char b[]);
static void rtsig_handler(int sig, siginfo_t *si, void *ucontext);
static void usrsig_handler();
void selectFifos(char **fifos, char **allFiles, int numberOfLines);
void customSleep(int nanoSecs);
void printHelp();
void printRaport();

int id = 10;
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
    int id;
};

struct status *wyslane;

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

    struct sigaction rtsa;
    sigemptyset(&rtsa.sa_mask);
    rtsa.sa_sigaction = rtsig_handler;
    rtsa.sa_flags = SA_SIGINFO;
    sigaction(sigNum, &rtsa, NULL);

    struct sigaction usrsa;
    memset(&usrsa, '\0', sizeof(usrsa));
    usrsa.sa_sigaction = usrsig_handler;
    usrsa.sa_flags = 0;
    sigaction(SIGUSR1, &usrsa, NULL);
    
    int stat = 0;
    while(1)
    {
        int nIt = 0;
        int nSize = 1;
        int numBytes = 0;
        int los = 0;
        int n = countFifos(allFiles, numberOfLines);
        char *fifos[n];
        int *fifoFds = (int*)malloc(1*sizeof(int));
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
                    unlink(fifos[i]);
                    fifos[i] = NULL;
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

        for(int i = 0; i<nSize-1; i++)
        {
            numBytes = 0;
            
            if( stat == 0 || oplacone(fifoFds[i]) )
            {
                numBytes = write(fifoFds[i], &paczka, sizeof(paczka));
            }

            if(numBytes == sizeof(paczka))
            {
                zapisane.fd = fifoFds[i];
                zapisane.status = 0;
                zapisane.id += 10;
                wyslane = (struct status*)realloc(wyslane, (iloscWydanych+1)*sizeof(*wyslane));
                wyslane[iloscWydanych] = zapisane;

                iloscWydanych++;
                paczka.idTowaru = iloscWydanych;
                close(fifoFds[i]);
                customSleep(4);
            }
        }
        stat = 1;
        if(isDone(fifos, n))
        {
            kill(getpid(), SIGUSR1);
        }
    }

    close(fd);
    return 0;
}

void rtsig_handler(int sig, siginfo_t *si, void *ucontext)
{
    for(int i = 0; i<iloscWydanych; i++)
    {
        if(si->si_value.sival_int == i)
        {
            wyslane[i].status = 1;
        }
    }
}

void usrsig_handler()
{
    //write(1, "FLAGA", 5);
    printRaport();
    exit(EXIT_SUCCESS);
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

int isDone(char **fifos, int n)
{
    for(int i = 0; i<n; i++)
    {
        if(fifos[i] != NULL)
        {
            return 0;
        }
    }

    return 1;
}

void customSleep(int nanoSecs)
{
    struct timespec tm;
    tm.tv_sec = 0;
    tm.tv_nsec = (nanoSecs * 100000000);
    nanosleep(&tm, NULL);
}

char *customSprintf(long i, char b[])
{
    char const digit[] = "0123456789";
    char* p = b;

    if(i<0)
    {
        *p++ = '-';
        i *= -1;
    }
    long shifter = i;

    do
    {
        ++p;
        shifter = shifter/10;
    }while(shifter);
    
    *p = '\0';
    do
    {
        *--p = digit[i%10];
        i = i/10;
    }while(i);
    
    return b;
}

void printHelp()
{
    printf("Przyklady uzycia:\n");
    printf("\t ./<sprzedawca> -s <zaplata[34-64]> <plik_konfiguracyjny>\n");
    printf("\t ./<sprzedawca> <plik_konfiguracyjny> -s <zaplata[34-64]>\n");
}

void printRaport()
{
    write(1, "RAPORT DZIAŁANIA\n", sizeof("RAPORT DZIAŁANIA\n"));
    write(1, "Sprzedano towarow: ", sizeof("Sprzedano towarow: ")/sizeof(char));
    char iloscChar[4];
    customSprintf(iloscWydanych, iloscChar);
    write(1, iloscChar, sizeof(iloscChar)/sizeof(char));
    write(1, "\n", sizeof("\n"));
    write(1, "LISTA SPRZEDANYCH\n", sizeof("LISTA SPRZEDANYCH\n"));

    for(int i = 0; i<iloscWydanych; i++)
    {
        char tempFd[4];
        customSprintf(wyslane[i].fd, tempFd);
        write(1, "ID TOWARU: ", sizeof("ID TOWARU: "));
        write(1, tempFd, sizeof(tempFd));
        write(1, "\t", sizeof("\t"));
        write(1, "STATUS: ", sizeof("STATUS: "));
        if(wyslane[i].status == 1)
            write(1, "OPLACONY\n", sizeof("OPLACONY\n"));
        else
            write(1, "NIEOPLACONY\n", sizeof("NIEOPLACONY\n"));
    }
}

