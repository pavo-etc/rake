#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LEN 100
#define	CHECK_ALLOC(p) if(p == NULL) { perror(__func__); exit(EXIT_FAILURE); } 

extern  char    **strsplit(const char *line, int *nwords);
extern  void    free_words(char **words);


char **hosts; 
int nhosts;
bool verbose;

// struct for commands & associated files
typedef struct {
    char *command;
    int n_required_files;
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