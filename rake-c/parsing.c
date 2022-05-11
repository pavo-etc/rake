#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

char *hosts[]; 

// struct for commands & associated files
typedef struct {
    char *command;
    int num_of_req_files;
    char *required_files[];
}COMMAND;

COMMAND actionsets[];

void read_file(int argc, char *argv[]){
    char *filename = "./Rakefile";
    if (argc > 1) {
        filename = argv[1];
    }

    printf("Reading from %s", filename);
    FILE *f = fopen(filename, "r");
    if (f == NULL){
        printf("Error opening the file");
        
        //Program exits gracefully if file pointer is NULL
        exit(1);
    }
    
    char line[100];
    char rakefile_lines[10000]; //not sure if size is adequate
    int count = 0;
    while(fgets(line, sizeof(line), f)) {
        printf(line);
        // I don't believe i am adding correctly here
        rakefile_lines[count] = line;
        count++;
    }
    fclose(f);

    char default_port[10];
    bool in_actionset = false;
    char current_actionset;

}

int main(int argc, char *argv[]){
    read_file(argc, argv);
    return 0;
}

