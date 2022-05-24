#include "rake-c.h"

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

        char **words = strsplit(line, &nwords);

        if (startswith("PORT", line)) {
            // Process port
            default_port = strdup(words[2]);
            CHECK_ALLOC(default_port);
            port_len = strlen(default_port);

        } else if (startswith("HOSTS", line)) {
            // Process hosts
            for (int i = 0; i < nwords-2; i++) {

                
                char *given_host = strdup(words[i+2]);
                CHECK_ALLOC(given_host);
                if (strchr(given_host, ':') == NULL) {
                    char *combined = calloc(1, sizeof(char) * (port_len + strlen(given_host) + 1)); //how did you come up with this???????????????????
                    strcpy(combined, given_host);
                    strcat(combined, ":");
                    strcat(combined, default_port);
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * 50); 
                    CHECK_ALLOC(hosts);
                    hosts[nhosts] = strdup(combined);
                    CHECK_ALLOC(hosts[nhosts]);
                    nhosts++;
                } else {
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
            CHECK_ALLOC(actionsets[last_actionset_index].commands);

            actionsets[last_actionset_index].commands[ncommands].command = strdup(line + 1);
            CHECK_ALLOC(actionsets[last_actionset_index].commands[ncommands].command);
            actionsets[last_actionset_index].commands[ncommands].requires_files = false;
            actionsets[last_actionset_index].ncommands++;

        } else if (strlen(line) < 2) {
            // blank line
        } else {
            // add new actionset  
            actionsets = realloc(
                actionsets,
                (nactionsets+1) * sizeof(ACTIONSET)
            );
            CHECK_ALLOC(actionsets);
            
            char *colonindex;
            //ignores the colon in the rakefile
            if ( (colonindex = strchr(line, ':')) != NULL ) {
                colonindex[0] = '\0';
            }
            actionsets[nactionsets].name = strdup(line);
            CHECK_ALLOC(actionsets[nactionsets].name);
            actionsets[nactionsets].ncommands = 0;
            actionsets[nactionsets].commands = create_empty_command();

            nactionsets++;

        }
        free_words(words);
    }

    fclose(f);

}

void print_actionsets() {
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