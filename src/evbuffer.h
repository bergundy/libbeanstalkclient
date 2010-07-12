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

#define EVBUFFER_FULL(buf) ( (buf)->writep->data != NULL )
#define EVBUFFER_EMPTY(buf) ( (buf)->readp->data == NULL || (buf)->readp->read )
#define EVBUFFER_HASCB(buf) ( (buf)->cbp->data == NULL )

struct _evbuffer_node {
    size_t length;
    off_t  offset;
    char   *data;
    bool   auto_free;
    bool   read;
    size_t bytes_expected;
    void (*cb)( struct _evbuffer_node *node, char *);
    struct _evbuffer_node *next;
};

typedef struct _evbuffer_node evbuffer_node;

typedef void (*callback_p_t)(evbuffer_node *node, char *);

struct _evbuffer {
    evbuffer_node *writep;
    evbuffer_node *readp;
    evbuffer_node *cbp;
    size_t length;
};

typedef struct _evbuffer evbuffer;

inline void evbuffer_cb_fin(evbuffer *buf);
inline void evbuffer_read_fin(evbuffer *buf);
inline evbuffer_node *evbuffer_getcb(evbuffer *buf);
inline evbuffer_node *evbuffer_get(evbuffer *buf);
inline int evbuffer_put(evbuffer *buf, char *msg, size_t msg_len, callback_p_t cb, bool auto_free);
void evbuffer_free(evbuffer *buf);
evbuffer *evbuffer_new(size_t init_len);

#endif /* EVBUFFER_H */
