#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAX_LINE_LEN 100
#define	CHECK_ALLOC(p) if(p == NULL) { perror(__func__); exit(EXIT_FAILURE); } 

extern  char    **strsplit(const char *line, int *nwords);
extern  void    free_words(char **words);

char *default_port;
char **hosts; 
int nhosts;
bool verbose;

// struct for commands & associated files
typedef struct {
    char *command;
    bool requires_files;
    char *required_files;
} COMMAND;

typedef struct {
    char *name;
    int ncommands;
    COMMAND *commands;
} ACTIONSET;

ACTIONSET *actionsets;
int nactionsets;


extern ACTIONSET *create_empty_actionset();
extern COMMAND *create_empty_command();
