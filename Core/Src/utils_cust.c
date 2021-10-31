/*
 * utils.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */
#include <stdlib.h>
#include <string.h>
#include "utils_cust.h"

void concatenate(char* dest,const char *a, const char *b, const char *c) {
    size_t alen = strlen(a);
    size_t blen = strlen(b);
    size_t clen = strlen(c);
    if (dest) {
        memcpy(dest, a, alen);
        memcpy(dest + alen, b, blen);
        memcpy(dest + alen + blen, c, clen + 1);
    }
}
