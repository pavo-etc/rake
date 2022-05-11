#include "rake-c.h"

ACTIONSET *create_empty_actionset(void) {
    ACTIONSET *new = calloc(1, sizeof(ACTIONSET *));
    CHECK_ALLOC(new);
    return new;
}