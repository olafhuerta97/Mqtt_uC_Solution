/*
 * utils.c
 *
 *  Created on: Oct 27, 2021
 *      Author: Olaf
 */
#include "MQTT_main.h"
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

unsigned char Atoi_Cust(char* str, unsigned short len, unsigned long *integer_result)
{
    /*Initialize result */
    int res = 0;
    unsigned char succeed = TRUE;

    /* Iterate through all characters
     of input string and update result
     take ASCII character of corresponding digit and
     subtract the code from '0' to get numerical
     value and multiply res by 10 to shuffle
     digits left to update running total */
    for (int i = 0; i< len; ++i)
    {
    	if (str[i] < '0' || str[i] > '9')
    	{
    		succeed = FALSE;
    		break;
    	}
        res = res * 10 + str[i] - '0';
    }

    /* return result.*/
    *integer_result = res;
    return succeed;
}
