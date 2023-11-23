/*
 * query_string.c
 *
 *  Created on: Nov 19, 2023
 *      Author: ngocrc
 */
#include "query_string.h"

uint8_t getKey(uint8_t* queryString, const char* key, uint8_t* value) {
    uint8_t* temp = NULL;
    temp = (uint8_t*)strstr((char*)queryString, key);
    if (temp == NULL)
        return 1;
    temp = temp + strlen(key);
    int i = 0;
    while (temp[i] != '&' && temp[i] != '\0') {
        value[i] = temp[i];
        i++;
    }
    value[i] = '\0';
    return 0;
}
