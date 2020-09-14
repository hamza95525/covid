#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h> //memset
#include <signal.h>
#include <poll.h>
#include "obslugaPliku.h"

void printHelp();

struct reklama
{
    pid_t PID;
    int sigNum;
    short idUlotki; 
};

int main(int argc, char* argv[])
{
    int opt = 0;
    int sigNum = 0;
    int amount = 0;
    char *pathToFile;
    struct reklama ulotka;
    memset(&ulotka, 0, sizeof(ulotka));

    if( (argc == 6 && argv[1][1] == 's') && (argc == 6 && argv[3][1] == 'r') )  
    {
        pathToFile = argv[5];
    }
    else if( (argc == 6 && argv[1][1] == 'r') && (argc == 6 && argv[3][1] == 's') )
    {
        pathToFile = argv[5];
    }
    else if( (argc == 6 && argv[1][1] == 'r') && (argc == 6 && argv[4][1] == 's') )
    {
        pathToFile = argv[3];
    }
    else if( (argc == 6 && argv[1][1] == 's') && (argc == 6 && argv[4][1] == 'r') )
    {
        pathToFile = argv[3];
    }
    else if( (argc == 6 && argv[1][1] != 's') && (argc == 6 && argv[1][1] != 'r')) 
    {
        pathToFile = argv[1];
    }
    else if( (argc == 6 && argv[3][1] == 's') && (argc == 6 && argv[3][1] != 'r')) 
    {
        pathToFile = argv[3];
    }
    else
    {
        printf("Zla liczba parametrow.\n");
        printHelp();
        exit(EXIT_FAILURE);
    }

    while( (opt = getopt(argc, argv, "s:r:")) != -1 )
    {
        switch(opt)
        {
            case 's':
                sigNum = strtol(optarg, NULL, 10);
                if(sigNum < 34 || sigNum > 64)
                {
                    printf("Zly numer sygnalu. \"kill -l | grep SIGRT\" - aby sprawdzic mozliwe numery.\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'r':
                amount = strtol(optarg, NULL, 10);
                break;
            default:
                printHelp();
                break;
        }
    }

    printf("%s ilosc: %d sygnal: %d\n", pathToFile, amount, sigNum);
}

void printHelp()
{
    printf("Przyklady uzycia:\n");
    printf("\t ./<kolporter> -s <potwierdzienie[34-64]> -r <ilosc_ulotek> <plik_konfiguracyjny>\n");
    printf("\t ./<kolporter> <plik_konfiguracyjny> -s <potwierdzenie[34-64]> -r <ilosc_ulotek>\n");
    printf("\t ./<kolporter> -r <ilosc_ulotek> -s <potwierdzienie[34-64]> <plik_konfiguracyjny>\n");
    printf("\t ./<kolporter> <plik_konfiguracyjny> -r <ilosc_ulotek> -s <potwierdzenie[34-64]>\n");
    printf("\t ./<kolporter> -r <ilosc_ulotek> <plik_konfiguracyjny> -s <potwierdzenie[34-64]>\n");
    printf("\t ./<kolporter> -s <potwierdzenie[34-64]> <plik_konfiguracyjny> -r <ilosc_ulotek>\n");
}