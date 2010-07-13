/**
 * =====================================================================================
 * @file   evbuffer.h
 * @brief  header file for 
 * @date   06/20/2010 02:18:12 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */
#ifndef EVBUFFER_H
#define EVBUFFER_H 

#include <stdbool.h>

#define EVBUFFER_FULL(q)        ( (q)->input_p == (q)->output_p )
#define EVBUFFER_EMPTY(q)       ( (q)->input_p == (q)->output_p->next )

typedef void (*callback_p_t)(evbuffer_node *node, char *);

struct _evbuffer_node {
    size_t length;
    char   *data;
    bool   autofree;
    size_t bytes_expected;
    void (*cb)( struct _evbuffer_node *node, char *);
    struct _evbuffer_node *next;
};

typedef struct _evbuffer_node evbuffer_node;

struct _evbuffer {
    evbuffer_node *writep;
    evbuffer_node *readp;
    size_t length;
};

typedef struct _evbuffer evbuffer;

inline void evbuffer_fin(evbuffer *buf);
inline evbuffer_node *evbuffer_get(evbuffer *buf);
inline int evbuffer_put(evbuffer *buf, char *msg, size_t msg_len, callback_p_t cb, bool autofree);
void evbuffer_free(evbuffer *buf);
evbuffer *evbuffer_new(size_t init_len);

#endif /* EVBUFFER_H */
