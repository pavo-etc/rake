#include "rake-c.h"

ACTIONSET *create_empty_actionset(void) {
    ACTIONSET *new = calloc(1, sizeof(ACTIONSET));
    CHECK_ALLOC(new);
    return new;
}

COMMAND *create_empty_command(void) {
    COMMAND *new = calloc(1, sizeof(COMMAND));
    CHECK_ALLOC(new);
    return new;
}