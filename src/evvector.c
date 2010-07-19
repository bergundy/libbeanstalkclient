/**
 * =====================================================================================
 * @file   evvector.c
 * @brief  evvector
 * @date   07/05/2010 06:23:57 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <stdlib.h>
#include "evvector.h"

evvector *evvector_new(size_t init_size)
{
    evvector *vec = NULL;

    if ( ( vec = (evvector *)malloc(sizeof(evvector) ) ) == NULL )
        return NULL;

    vec->data = NULL;

    if ( ( vec->data = (char *)malloc( sizeof(char) * init_size ) ) == NULL ) {
        free(vec);
        return NULL;
    }

    vec->som = vec->eom = vec->data;
    vec->size           = init_size;

    return vec;
}

void evvector_free(evvector *vec)
{
    free(vec->data);
    free(vec);
}

bool evvector_expand(evvector *vec)
{
    char *realloc_data = NULL;

    if ( ( realloc_data = (char *)realloc( vec->data, sizeof(char) * vec->size * 2 ) ) == NULL )
        return false;

    vec->som  = vec->som - vec->data + realloc_data;
    vec->eom  = vec->eom - vec->data + realloc_data;
    vec->data = realloc_data;
    vec->size *= 2;

    return true;
}
