#include "rake-c.h"

bool startswith(const char *prefix, const char *s) {
    return strncmp(prefix, s, strlen(prefix)) == 0;
}

void slice(const char *in, char *out, size_t start, size_t end) {
    strncpy(out, in + start, end - start);
}


void read_file(char *filename) {


    printf("Reading from %s\n", filename);
    FILE *f = fopen(filename, "r");
    //exit gracefully if file descriptor is equal to null
    if (f == NULL){
        fprintf(stderr, "Error opening the file %s\n", filename);
        exit(EXIT_FAILURE);
    }
    actionsets = calloc(1, sizeof(ACTIONSET));
    CHECK_ALLOC(actionsets);

    char *default_port;
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
        printf("%s\n", line);
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
            int ncommands = actionsets[last_actionset_index].ncommands;
            
            char *spaceindex;
            if ( (spaceindex = strchr(line, ' ')) != NULL ) {
                actionsets[last_actionset_index].commands[ncommands].required_files = strdup(spaceindex + 1);
                printf("required_files: %s\n", actionsets[last_actionset_index].commands[ncommands].required_files);
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
            actionsets[last_actionset_index].commands[ncommands].n_required_files = 0;
            actionsets[last_actionset_index].commands[ncommands].required_files = calloc(1, sizeof(char **));
            //CHECK_ALLOC(actionsets[last_actionset_index].commands[ncommands].required_files);
            printf("\tAdded command %d\n", actionsets[last_actionset_index].ncommands);
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

            
            printf("New action set created: %s\n", actionsets[nactionsets].name);
            nactionsets++;

        }
        free_words(words);
    }

    fclose(f);
    
    printf("default_port: %s\n", default_port);
    for (int i = 0; i < nhosts; i++) {
        printf("hosts[%d]: %s\n", i, hosts[i]);
    }

    for (int i = 0; i < nactionsets; i++) {
        printf("%s\n",actionsets[i].name);
        int ncommands = actionsets[i].ncommands;
        for (int j = 0; j < ncommands; j++) {
            printf("\t%s\n", actionsets[i].commands[j].command);

        }

    }


}

int main(int argc, char *argv[]) {
    char *filename = "./Rakefile";
    if (argc > 1) {
        filename = argv[1];
    }

    //actionsets = create_empty_actionset();
    read_file(filename);
    exit(EXIT_SUCCESS);
}

