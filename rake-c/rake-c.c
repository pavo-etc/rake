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

int strtokcount(char *string, char *delim) {
    char *token = strtok(strdup(string), delim);
    int count = 0;

    while (token != NULL) {
        count++;
        token = strtok(NULL, delim);
    }
    return count;
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
    //exit gracefully if file pointer is equal to null
    if (f == NULL){
        fprintf(stderr, "Error opening the file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    actionsets = calloc(1, sizeof(ACTIONSET)+1);
    CHECK_ALLOC(actionsets);

    int port_len = 0;
    char *line = NULL;
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
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * 50); 
                    CHECK_ALLOC(hosts);
                    hosts[nhosts] = strdup(combined);
                    CHECK_ALLOC(hosts[nhosts]);
                    nhosts++;
                } else {
                    //printf("Has port!\n");
                    // this is sus since given_host could be smaller than previous.  should replace strlen(given_host) with something that will always increase size
                    //once again couldn't we do an if, but then i suppose we could be wasting space if we get somthing smaller
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * 50);
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
    // big endian nbytes
    uint32_t be_nbytes = htonl(nbytes);
    printf("\tSending %d\n",nbytes);
    send(sock, &be_nbytes, 4, 0);
    send(sock, msg, nbytes, 0);
}

void *recv_msg(int sock, int *size) {
    uint32_t *msg_len_ptr = calloc(1, 4);
    int nbytes = recv(sock, msg_len_ptr, 4, 0);
    if (nbytes == 0) return NULL;

    uint32_t msg_len = ntohl(*msg_len_ptr);
    *size = msg_len;
    void *msg = calloc(1, msg_len);
    recv(sock, msg, msg_len, 0);

    return msg;
}

int create_socket(char *hostname, char *port) {
    struct addrinfo hints, *res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;//AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    
    if ((status = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(1);
    }

    int s = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    connect(s, res->ai_addr, res->ai_addrlen);
    return s;
}

int find_host() {
    int mincost = 1000;
    int best_host_index = 0;
    char *host;
    char *port;
    char colon[2] = ":";
    char space[2] = " ";
    for (int i = 0; i < nhosts; i++) {
        host = strtok(strdup(hosts[i]), colon);
        port = strtok(NULL, colon);
        printf("\tQuerying cost from host %d %s\n", i, hosts[i]);

        int sock = create_socket(host, port);
        char *msg = "cost-query";

        send_msg(sock, msg, strlen(msg));
        printf("\t\t--> %s\n", (char *)msg);
        int msgsize; 
        char *recved_data = (char *) recv_msg(sock, &msgsize);
        printf("\t\t<-- %s\n", recved_data);

        if ( strcmp(strtok(recved_data, space), "cost") == 0 ) {
            int cost = atoi(strtok(NULL, space));

            if (cost < mincost) {
                best_host_index = i;
                mincost = cost;
            }

        } else {
            fprintf(stderr, "Couldn't retrieve cost from %s:%s\n", host, port); 
            exit(EXIT_FAILURE);
        }

    }
    printf("\t\tSelecting %s\n", hosts[best_host_index]);
    return best_host_index;
}

int send_command(int cmdindex, COMMAND *cmddata) {
    //printf("command index %d %s\n", i, cmddata->command);
    char *hostname;
    char *port;
    char colon[2] = ":";
    char space[2] = " ";
    
    if (startswith("remote-", cmddata->command)) {
        char* host = strdup(hosts[find_host()]);
        hostname = strtok(host, colon);
        port = strtok(NULL, colon);
    } else {
        hostname = "localhost";
        port = default_port;
    }
    printf("\tAttempting connection to %s:%s\n", hostname, port);
    int sock = create_socket(hostname, port);
    int n_required_files = 0;
    if (cmddata->requires_files) {
        n_required_files = strtokcount(cmddata->required_files, " ");
    }
    size_t buf_size =  snprintf(NULL, 0, "%d %d %s", cmdindex, n_required_files, cmddata->command);

    char *msg = calloc(1,buf_size+1);
    sprintf(msg, "%d %d %s", cmdindex, n_required_files, cmddata->command);
    send_msg(sock, msg, strlen(msg));
    printf("\t\t--> %s\n", msg);

    if (cmddata->requires_files) {
        char *filename;
        struct stat st;
        size_t filesize;
        //int fd;
        void *msg;
        //int n = 0;
                
        for (int i = 0; i < n_required_files; i++) {
            if (i == 0) {
                filename = strtok(cmddata->required_files, space);
            } else {
                filename = strtok(NULL, space);
            }
            send_msg(sock, filename, strlen(filename));
            printf("\t\t--> %s\n", filename);
            if (stat(filename, &st) != 0) {
                fprintf(stderr, "Error: could not find file %s", filename);
                exit(EXIT_FAILURE);
            }
            filesize = st.st_size;
            printf("\t\tfilesize %zu\n", filesize);
            msg = calloc(1, filesize+1);
            //fd = open(filename, O_RDONLY);
            //n = read(fd, msg, filesize);
            FILE *fp = fopen(filename, "rb");
            int nbytes = fread(msg, 1, filesize, fp);
            printf("\t\tRead %d bytes\n", nbytes);
            send_msg(sock, msg, filesize);
            printf("\t\t--> file (size %zu)\n", filesize);
            //close(fd);
            fclose(fp);
        }

    }

    return sock;

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
        int sockets[ncmds];
        char *stdouts[ncmds];
        char *stderrs[ncmds];
        int exitcodes[ncmds];

        for (int j = 0; j < ncmds; j++) {
            if (verbose) printf("%s\n", cmdlist[j].command);
            sockets[j] = send_command(j, &cmdlist[j]);
            printf("\tsocketfd %d\n", sockets[j]);
        }

        //struct timeval tv = {1, 1000000};

        fd_set read_fds;
        int max_fd = 0;
        int cmds_returned = 0;
        int msgsize = 0;
        while (true) {
            FD_ZERO(&read_fds);
            
            for (int j = 0; j < ncmds; j++) {
                FD_SET(sockets[j], &read_fds);
                if (sockets[j] > max_fd) max_fd = sockets[j];
            }
            if (select(max_fd+1, &read_fds, NULL, NULL, NULL) == -1) {
                perror("select() error\n");
                exit(1);
            }
             
            for (int j = 0; j < ncmds; j++) {
                //printf("sockets[%d]=%d  %d\n", j ,sockets[j],FD_ISSET(sockets[j], &read_fds));
                if (FD_ISSET(sockets[j], &read_fds)) {
                    char *msg1 = recv_msg(sockets[j], &msgsize);
                    if (msg1 == NULL) {
                        //printf("socket %d closed\n", j);
                        continue;
                    }

                    char *msg2 = recv_msg(sockets[j],  &msgsize);
                    char *msg3 = recv_msg(sockets[j],  &msgsize);

                    char space[2] = " ";

                    int cmdindex = atoi(strtok(strdup(msg1), space));
                    int exitcode = atoi(strtok(NULL, space));
                    int nfiles = atoi(strtok(NULL, space));


                    exitcodes[j] = exitcode;
                    stdouts[j] = msg2;
                    stderrs[j] = msg3;
                    printf("\t<-- cmdindex exitcode nfile %d %d %d\n", cmdindex, exitcode, nfiles);
                    printf("\t<-- stdout (length %ld)\n", strlen(msg2));
                    printf("\t<-- stderr (length %ld)\n", strlen(msg3));
                    cmds_returned++;

                    if (nfiles > 0) {
                        char *filename = recv_msg(sockets[j], &msgsize);
                        msgsize = 0;
                        void *filecontents = recv_msg(sockets[j], &msgsize);
                        FILE *fp = fopen(filename, "wb");
                        fwrite(filecontents, msgsize, 1,  fp);
                        printf("\t<-- %s\n", filename);
                        printf("\t<-- file (%d)\n", msgsize);
                        printf("\tWrote file %s\n", filename);
                        fclose(fp);
                    }
                }
            }
            if (cmds_returned >= ncmds) {
                break;
            }

        }
        for (int j = 0; j < ncmds; j++) {
            printf("Command %d exitcode: %d\n", j, exitcodes[j]);
            printf("Command %d stdout: \n%s\n", j, stdouts[j]);
            printf("Command %d stderr: \n%s\n", j, stderrs[j]);
            if (exitcodes[j] != 0) {
                fprintf(stderr, "Error: command failed:\n\tactionset: %s\n\tcommand: %s\n\texitcode: %d\n\tstderr: %s",
                    actionsetname,
                    cmdlist[j].command,
                    exitcodes[j],
                    stderrs[j]
                );
                exit(EXIT_FAILURE);
            }
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
    hosts = NULL;
    actionsets = NULL;
    char *filename = "./Rakefile";

    if (optind < argc) {
        filename = argv[optind];
    }

    read_file(filename);
    execute_actionsets();
    exit(EXIT_SUCCESS);
}

