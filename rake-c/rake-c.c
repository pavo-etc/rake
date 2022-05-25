#include "rake-c.h"
#include <getopt.h>

/*
"main" file to run the C rake client
*/

void usage(char *argv[]) {
    fprintf(stderr, "Usage: %s [-v] [file]\n\t-v: Verbose output\n\tfile: file to execute instead of \"./Rakefile\"\n",
        argv[0]
    );
    exit(EXIT_FAILURE);
}

// Used for when a missing file is detected
void missing_file(COMMAND *cmddata, char *filename) {
    fprintf(stderr, "Error: missing file\n\tcommand: %s\n\tfilename: %s", 
        cmddata->command, 
        filename
    );
    exit(EXIT_FAILURE);
}

// Used for when a file fails to open
void open_file_fail(COMMAND *cmddata, char *filename) {
    fprintf(stderr, "Error: couldn't open file\n\tcommand: %s\n\tfilename: %s", 
        cmddata->command, 
        filename
    );
    exit(EXIT_FAILURE);
}

// Used for when a command fails
void command_fail(char *actionsetname, char *cmd, int exitcode, char *stderr_txt) {
    fprintf(stderr, "Error: command failed\n\tactionset: %s\n\tcommand: %s\n\texitcode: %d\n\tstderr: %s", 
        actionsetname, 
        cmd, 
        exitcode, 
        stderr_txt
    );
    exit(EXIT_FAILURE);
}

// Chooses the host with the lowest cost after sending a cost-query to each host
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
        if (verbose) printf("\tQuerying cost from host %d %s\n", i, hosts[i]);

        int sock = create_socket(host, port);

        // Send a query to the host
        char *msg = "cost-query";
        send_msg(sock, msg, strlen(msg));
        if (verbose) printf("\t\t--> %s\n", (char *)msg);
        
        int msgsize; 

        // Receive the cost
        char *recved_data = (char *) recv_msg(sock, &msgsize);
        if (verbose) printf("\t\t<-- %s\n", recved_data);

        if ( strcmp(strtok(recved_data, space), "cost") == 0 ) {
            int cost = atoi(strtok(NULL, space));

            if (cost < mincost) {
                best_host_index = i;
                mincost = cost;
            }

        } else {
            // This should never be possible to execute
            fprintf(stderr, "Couldn't retrieve cost from %s:%s\n", host, port); 
            exit(EXIT_FAILURE);
        }
        close(sock);
    }
    if (verbose) printf("\t\tSelecting %s\n", hosts[best_host_index]);

    // Returns the index of the host with lowest cost in the list of hosts
    return best_host_index;
}

// Sends command to host with lowest cost to be processed
int send_command(int cmdindex, COMMAND *cmddata) {
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

    if (verbose) printf("\tAttempting connection to %s:%s\n", hostname, port);
    int sock = create_socket(hostname, port);

    int n_required_files = 0;
    if (cmddata->requires_files) {
        n_required_files = strtokcount(cmddata->required_files, " ");
    }

    size_t buf_size =  snprintf(NULL, 0, "%d %d %s", cmdindex, n_required_files, cmddata->command);

    char *msg = calloc(1,buf_size+1);
    sprintf(msg, "%d %d %s", cmdindex, n_required_files, cmddata->command);
    send_msg(sock, msg, strlen(msg));
    if (verbose) printf("\t\t--> %s\n", msg);

    if (cmddata->requires_files) {
        char *filename;
        struct stat st;
        size_t filesize;
        void *msg;
        FILE *fp;  
        for (int i = 0; i < n_required_files; i++) {
            
            if (i == 0) filename = strtok(cmddata->required_files, space);
            else filename = strtok(NULL, space);
            
            send_msg(sock, filename, strlen(filename));
            if (verbose) printf("\t\t--> %s\n", filename);

            if (stat(filename, &st) != 0) missing_file(cmddata, filename);
            filesize = st.st_size;
            msg = calloc(1, filesize+1);
            
            fp = fopen(filename, "rb");
            
            int fread_exit = fread(msg, 1, filesize, fp);
            if ( (fread_exit) < 0 ) open_file_fail(cmddata, filename); 
            
            fclose(fp);
            
            send_msg(sock, msg, filesize);
            if (verbose) printf("\t\t--> file (size %zu)\n", filesize);
        }
    }
    return sock;
}

// Function to execute all the actionsets in the actionsets in the Rakefile
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
        }

        if (verbose) printf("Waiting for command execution results...\n");

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
            
            // loop for obtaining the data received back from the server
            for (int j = 0; j < ncmds; j++) {
                if (FD_ISSET(sockets[j], &read_fds)) {
                    
                    char *msg1 = recv_msg(sockets[j], &msgsize);
                    if (msg1 == NULL) continue;

                    char *msg2 = recv_msg(sockets[j],  &msgsize);
                    char *msg3 = recv_msg(sockets[j],  &msgsize);

                    char space[2] = " ";

                    int cmdindex = atoi(strtok(strdup(msg1), space));
                    int exitcode = atoi(strtok(NULL, space));
                    int nfiles = atoi(strtok(NULL, space));


                    exitcodes[j] = exitcode;
                    stdouts[j] = msg2;
                    stderrs[j] = msg3;
                    if (verbose) {
                        printf("\t<-- cmdindex exitcode nfile %d %d %d\n", cmdindex, exitcode, nfiles);
                        printf("\t<-- stdout (length %ld)\n", strlen(msg2));
                        printf("\t<-- stderr (length %ld)\n", strlen(msg3));
                    }
                    cmds_returned++;

                    if (nfiles > 0) {
                        char *filename = recv_msg(sockets[j], &msgsize);
                        msgsize = 0;
                        void *filecontents = recv_msg(sockets[j], &msgsize);
                        FILE *fp = fopen(filename, "wb");
                        fwrite(filecontents, msgsize, 1,  fp);
                        fclose(fp);
                        if (verbose) {
                            printf("\t<-- %s\n", filename);
                            printf("\t<-- file (%d)\n", msgsize);
                            printf("\tSaved file %s\n", filename);
                        }
                    }
                }
            }
            if (cmds_returned >= ncmds) {
                break;
            }
        }
        for (int j = 0; j < ncmds; j++) {
            if (verbose) {
                printf("Command %d exitcode: %d\n", j, exitcodes[j]);
                printf("Command %d stdout: \n%s\n", j, stdouts[j]);
                printf("Command %d stderr: \n%s\n", j, stderrs[j]);
            }
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
    // Check for verbose flag "-v"
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
    if (verbose) print_actionsets();
    execute_actionsets();
    if (verbose) printf("Rakefile executed successfully!\n");
    exit(EXIT_SUCCESS);
}

