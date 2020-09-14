#include "obslugaPliku.h"


int openFile(char* pathToFile)
{
    int fd = open(pathToFile, O_RDONLY);
    if(fd < 0)
    {
        perror("Can't open config file");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int getNumberOfFiles(int fd)
{
    char buffer[1];
    int lineCount = 0;

    while( read(fd, buffer, 1) != 0  )
    {
        if(buffer[0] == '\n')
        {
            lineCount += 1;
        }
    }

    lseek(fd, 0, SEEK_SET);
    return lineCount;
}

void getAllFiles(int fd, char **allFiles)
{
    char buffer[1];
    int charsCount = 0;
    int lineCount = 0;

    while( read(fd, buffer, 1) != 0  ) //pobieranie dlugosci poszczegolnych linii
    {
        if(buffer[0] == '\n')
        {
            allFiles[lineCount] = (char*)malloc(charsCount * sizeof(char));
            charsCount = 0;
            lineCount += 1;
            continue;
        }
        charsCount += 1;
    }
    lseek(fd, 0, SEEK_SET);

    lineCount = 0;
    charsCount = 0;
    while( read(fd, buffer, 1) != 0)
    {
        if(buffer[0] == '\n')
        {
            lineCount += 1;
            charsCount = 0;
            continue;
        }
        allFiles[lineCount][charsCount] = buffer[0];
        charsCount += 1;
    }
    lseek(fd, 0, SEEK_SET);
}

char *getRandomFile(char **allFiles, int numberOfLines)
{
    srand(time(NULL));
    int random = rand()%numberOfLines;

    char *randomFile = allFiles[random];
    while( (randomFile[0] == ' ' && randomFile[1] == ' ' ) || randomFile[0] == '\n')
    {
        random = rand()%numberOfLines;
        randomFile = allFiles[random];
    }

    return randomFile;
}