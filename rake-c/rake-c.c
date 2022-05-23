#include "rake-c.h"
#include <getopt.h>


void usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [-v] [file]\n\t-v: Verbose output\n\tfile: file to execute instead of \"./Rakefile\"\n",
        argv[0]
    );
    exit(EXIT_FAILURE);
}

bool startswith(const char *prefix, const char *s) {
    return strncmp(prefix, s, strlen(prefix)) == 0;
}

void slice(const char *in, char *out, size_t start, size_t end) {
    strncpy(out, in + start, end - start);
}

void missing_file(char *command_data, char *filename) {
    fprintf(stderr, "Error: missing file\n\tcommand: %d\n\tfilename: %s", command_data[0], filename);
    exit(EXIT_FAILURE);
}

void command_fail(char *actionset, char *command_data, int exitcode, char *stderr_txt) {
    fprintf(stderr, "Error: command failed\n\tactionset: %s\n\tcommand: %d\n\texitcode: %d\n\tstderr: %s", 
    actionset, command_data[0], exitcode, stderr_txt);
    exit(EXIT_FAILURE);
}

void read_file(char *filename) {

    if (verbose) printf("Reading from %s\n", filename); 

    FILE *f = fopen(filename, "r");
    //exit gracefully if file descriptor is equal to null
    if (f == NULL){
        fprintf(stderr, "Error opening the file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    actionsets = calloc(1, sizeof(ACTIONSET));
    CHECK_ALLOC(actionsets);

    int port_len = 0;
    char *line;
    size_t len = 0;
    size_t nread;
    int nwords;

    while ( (nread = getline(&line, &len, f) != -1) ) {


        char *commentstart;
        //ignores the comments in the rakefile
        if ( (commentstart = strchr(line, '#')) != NULL ) {
            commentstart[0] = '\0';
        }
        //replaces each new line character with the null byte
        size_t lastchr = strlen(line) - 1;
        if (line[lastchr] == '\n') {
            line[lastchr] = '\0';
        }


        //printf("%d : %s\n", nwords, line);
        //printf("strlen(line): %ld\n",strlen(line));
        //if (verbose) printf("%s\n", line); 
        char **words = strsplit(line, &nwords);
        //printf("words[0]: %s\n",words[0]);

        if (startswith("PORT", line)) {
            // Process port
            default_port = strdup(words[2]);
            CHECK_ALLOC(default_port);
            port_len = strlen(default_port);

        } else if (startswith("HOSTS", line)) {
            // Process hosts
            for (int i = 0; i < nwords-2; i++) {
                //for debugging purposes, assuming to make sure your accessing correct indices 
                //printf("i: %d words[i]: %s\n", i,words[i]);
                //printf("words[i+2]: %s\n", words[i+2]);
                
                char *given_host = strdup(words[i+2]);
                CHECK_ALLOC(given_host);
                //printf("given_host: %s\n", given_host);
                if (strchr(given_host, ':') == NULL) {
                    char *combined = calloc(1, sizeof(char) * (port_len + strlen(given_host) + 1)); //how did you come up with this???????????????????
                    strcpy(combined, given_host);
                    strcat(combined, ":");
                    strcat(combined, default_port);
                    //printf("No port!\n");
                    //printf("Added port %s\n", combined);
                    // this is sus since combined could be smaller than previous.  should replace strlen(combined) with something that will always increase size
                    //could you explain this please, if my understanding is right couldn't we simply do an if statement here to make sure it is not a decrease?
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * strlen(combined)); 
                    CHECK_ALLOC(hosts);
                    hosts[nhosts] = strdup(combined);
                    CHECK_ALLOC(hosts[nhosts]);
                    nhosts++;
                } else {
                    //printf("Has port!\n");
                    // this is sus since given_host could be smaller than previous.  should replace strlen(given_host) with something that will always increase size
                    //once again couldn't we do an if, but then i suppose we could be wasting space if we get somthing smaller
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * strlen(given_host));
                    CHECK_ALLOC(hosts);
                    hosts[nhosts] = strdup(given_host);
                    CHECK_ALLOC(hosts[nhosts]);
                    nhosts++;
                }

            }
        } else if (startswith("\t\t", line)) {
            // add required file to command
            int last_actionset_index = nactionsets-1;
            int last_command_index = actionsets[last_actionset_index].ncommands-1;

            char *spaceindex;
            if ( (spaceindex = strchr(line, ' ')) != NULL ) {
                actionsets[last_actionset_index].commands[last_command_index].required_files = strdup(spaceindex+1);
                CHECK_ALLOC(actionsets[last_actionset_index].commands[last_command_index].required_files);
                
                actionsets[last_actionset_index].commands[last_command_index].requires_files = true;
            } else {
                fprintf(stderr, "No space in required files line in %s\n", filename);
                exit(EXIT_FAILURE);
            }

        } else if (startswith("\t", line)) {
            // add command
            int last_actionset_index = nactionsets-1;
            int ncommands = actionsets[last_actionset_index].ncommands;
            
            actionsets[last_actionset_index].commands = realloc(
                actionsets[last_actionset_index].commands,
                (ncommands+1) * sizeof(COMMAND)
            );

            actionsets[last_actionset_index].commands[ncommands].command = strdup(line + 1);
            CHECK_ALLOC(actionsets[last_actionset_index].commands[ncommands].command);
            actionsets[last_actionset_index].commands[ncommands].requires_files = false;
            //CHECK_ALLOC(actionsets[last_actionset_index].commands[ncommands].required_files);
            //if (verbose) printf("\tAdded command %d\n", actionsets[last_actionset_index].ncommands); 
            actionsets[last_actionset_index].ncommands++;

            //printf("NEWCOMMAND: %s\n", actionsets[nactionsets].commands[ncommands].command);
        } else if (strlen(line) < 2) {
            //printf("Line empty\n");
            // blank line
        } else {
            //printf("nactionsets: %d\n", nactionsets);

            // add new actionset  
            actionsets = realloc(
                actionsets,
                (nactionsets+1) * sizeof(ACTIONSET)
            );
            
            char *colonindex;
            //ignores the comments in the rakefile
            if ( (colonindex = strchr(line, ':')) != NULL ) {
                colonindex[0] = '\0';
            }
            actionsets[nactionsets].name = strdup(line);
            CHECK_ALLOC(actionsets[nactionsets].name);
            actionsets[nactionsets].ncommands = 0;
            actionsets[nactionsets].commands = create_empty_command();

            
            //if (verbose) printf("New action set created: %s\n", actionsets[nactionsets].name); 
            nactionsets++;

        }
        free_words(words);
    }

    fclose(f);
    
    if (verbose) {
        printf("default_port: %s\n", default_port); 
        for (int i = 0; i < nhosts; i++) {
            printf("hosts[%d]: %s\n", i, hosts[i]); 
        }

        for (int i = 0; i < nactionsets; i++) {
            printf("%s\n",actionsets[i].name); 
            int ncommands = actionsets[i].ncommands;
            for (int j = 0; j < ncommands; j++) {
                printf("\t%s\n", actionsets[i].commands[j].command); 
                if (actionsets[i].commands[j].requires_files == true) {
                    printf("\t\tRequires files: %s\n", actionsets[i].commands[j].required_files);
                }
            }
        }
    }
}

void send_msg(int sock, void *msg, uint32_t nbytes) {
    // big endian nbyrtes
    uint32_t be_nbytes = htonl(nbytes);
    send(sock, &be_nbytes, 4, 0);
    send(sock, msg, nbytes, 0);
}

void *recv_msg(int sock) {
    //printf("Attempting msg len\n");
    uint32_t *msg_len_ptr = calloc(1, 4);
    recv(sock, msg_len_ptr, 4, 0);
    //printf("Recved msg len\n");

    uint32_t msg_len = ntohl(*msg_len_ptr);
    //printf("%d\n", msg_len);

    void *msg = calloc(1, msg_len);

    recv(sock, msg, msg_len, 0);

    return msg;
}

int create_socket(char *hostname, char *port) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(s, res->ai_addr, res->ai_addrlen);
    return s;
}

void send_command(int i, COMMAND *cmddata) {
    //printf("command index %d %s\n", i, cmddata->command);

    int sock = create_socket("localhost", "40000");
    char *msg = "cost-query";
    printf("\t--> %s\n", (char *)msg);

    send_msg(sock, msg, strlen(msg));
    char *cost = (char *) recv_msg(sock);
    printf("\t<-- %s\n", cost);
    //freeaddrinfo(res); // free the linked list

    //if (startswith("remote-", cmddata->command)) {}*/
}

void execute_actionsets() {
    char *actionsetname;
    COMMAND *cmdlist;
    int ncmds;
    for (int i = 0; i < nactionsets; i++) {
        actionsetname = actionsets[i].name;
        if (verbose) printf("------------Starting %s------------\n", actionsetname);
        cmdlist = actionsets[i].commands;
        ncmds = actionsets[i].ncommands;

        for (int j = 0; j < ncmds; j++) {
            if (verbose) printf("%s\n", cmdlist[j].command);
            send_command(j, &cmdlist[j]);


        }
    }
}

int main(int argc, char *argv[]) {
    
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
            case 'v':
                verbose = true;
                break;
            default:
                usage(argv);
        }
    }

    char *filename = "./Rakefile";

    if (optind < argc) {
        filename = argv[optind];
    }

    read_file(filename);
    execute_actionsets();
    exit(EXIT_SUCCESS);
}

