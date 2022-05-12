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

        printf("%d : %s\n", nwords, line);
        //printf("strlen(line): %ld\n",strlen(line));
        char **words = strsplit(line, &nwords);
        //printf("words[0]: %s\n",words[0]);
        if (startswith("PORT", line)) {
            // Process port
            default_port = strdup(words[2]);
            CHECK_ALLOC(default_port);
            port_len = strlen(default_port);

            printf("default_port=%s\n", default_port);
        } else if (startswith("HOSTS", line)) {
            // Process hosts
            for (int i = 0; i < nwords-2; i++) {
                //for debugging purposes, assuming to make sure your accessing correct indices 
                printf("i: %d words[i]: %s\n", i,words[i]);
                printf("words[i+2]: %s\n", words[i+2]);
                
                char *given_host = strdup(words[i+2]);
                CHECK_ALLOC(given_host);
                printf("given_host: %s\n", given_host);
                if (strchr(given_host, ':') == NULL) {
                    char *combined = calloc(1, sizeof(char) * (port_len + strlen(given_host) + 1)); //how did you come up with this???????????????????
                    strcpy(combined, given_host);
                    strcat(combined, ":");
                    strcat(combined, default_port);
                    printf("No port!\n");
                    printf("Added port %s\n", combined);
                    // this is sus since combined could be smaller than previous.  should replace strlen(combined) with something that will always increase size
                    //could you explain this please, if my understanding is right couldn't we simply do an if statement here to make sure it is not a decrease?
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * strlen(combined)); 
                    hosts[nhosts] = strdup(combined);
                    nhosts++;
                } else {
                    printf("Has port!\n");
                    // this is sus since given_host could be smaller than previous.  should replace strlen(given_host) with something that will always increase size
                    //once again couldn't we do an if, but then i suppose we could be wasting space if we get somthing smaller
                    hosts = realloc(hosts, (nhosts+1) * sizeof(char) * strlen(given_host));
                    CHECK_ALLOC(hosts);
                    hosts[nhosts] = strdup(given_host);
                    CHECK_ALLOC(hosts[nhosts])
                    nhosts++;
                }

            }
        } else if (startswith("\t\t", line)) {
            // add required file to command
            /*for (int i = 0; i < nwords-1; i++) {
                actionsets[nactionsets].commands = realloc(
                    actionsets[nactionsets].commands,
                    (nactionsets+1) * (sizeof(char *) + sizeof(int) + sizeof(COMMAND *))
                );
                CHECK_ALLOC(actionsets);
            }*/

        } else if (startswith("\t", line)) {
            // add command
        } else if (strlen(line) < 2) {
            printf("Line empty\n");
            // blank line
        } else {
            // add new actionset  
            // gonna be a problem since we aren't filtering out comments yet
            actionsets = realloc(
                actionsets,
                (nactionsets+1) * sizeof(ACTIONSET)
            );

            actionsets[nactionsets].name = line;
            actionsets[nactionsets].ncommands = 0;

            printf("New action set created: %s\n", actionsets[nactionsets].name);
            nactionsets++;

        }
        free_words(words);
    }
    printf("hosts[0]: %s\n", hosts[0]);
    printf("hosts[1]: %s\n", hosts[1]);
    printf("hosts[2]: %s\n", hosts[2]);

    fclose(f);
}

int main(int argc, char *argv[]) {
    char *filename = "./Rakefile";
    if (argc > 1) {
        filename = argv[1];
    }

    actionsets = create_empty_actionset();
    read_file(filename);
    exit(EXIT_SUCCESS);
}

