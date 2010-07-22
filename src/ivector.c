/**
 * =====================================================================================
 * @file   ivector.c
 * @brief  ivector - an extendable vector for storing read input
 * @date   07/05/2010 06:23:57 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <stdlib.h>
#include "ivector.h"

ivector *ivector_new(size_t init_size)
{
    ivector *vec = NULL;

    if ( ( vec = (ivector *)malloc(sizeof(ivector) ) ) == NULL )
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

void ivector_free(ivector *vec)
{
    free(vec->data);
    free(vec);
}

bool ivector_expand(ivector *vec)
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
