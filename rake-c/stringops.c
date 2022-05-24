#include "rake-c.h"

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
