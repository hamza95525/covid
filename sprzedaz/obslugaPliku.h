#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <time.h>

int openFile(char* pathToFile);
int getNumberOfFiles(int fd);
void getAllFiles(int fd, char **allLines);
char *getRandomFile(char **allLines, int numberOfLines);