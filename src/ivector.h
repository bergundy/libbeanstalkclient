/**
 * =====================================================================================
 * @file   ivector.h
 * @brief  header file for ivector
 * @date   07/05/2010 06:20:49 PM
 * @author Roey Berman, (roey.berman@gmail.com)
 * =====================================================================================
 */
#ifndef IVECTOR_H
#define IVECTOR_H 

#include <stdbool.h>

#define IVECTOR_FREE(vec) ( (vec)->size - ( (vec)->eom - (vec)->data ) - 1 )

struct _ivector {
    char   *data;
    char  *som;
    char  *eom;
    size_t size;
};

typedef struct _ivector ivector;

ivector *ivector_new(size_t init_size);
void     ivector_free(ivector *vec);
bool     ivector_expand(ivector *vec);

#endif /* IVECTOR_H */
