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

extern  char    **strsplit(const char *line, int *nwords);
extern  void    free_words(char **words);

// String ops
extern bool startswith(const char *prefix, const char *s);
extern void slice(const char *in, char *out, size_t start, size_t end);
extern int strtokcount(char *string, char *delim);

// File ops
extern void read_file(char *filename);
extern void print_actionsets();

// Network ops
extern void send_msg(int sock, void *msg, uint32_t nbytes);
extern void *recv_msg(int sock, int *size);
extern int create_socket(char *hostname, char *port);

extern ACTIONSET *create_empty_actionset();
extern COMMAND *create_empty_command();

char *default_port;
char **hosts; 
int nhosts;
bool verbose;

ACTIONSET *actionsets;
int nactionsets;
