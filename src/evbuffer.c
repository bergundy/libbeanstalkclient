/**
 * =====================================================================================
 * @file   evbuffer.c
 * @brief  
 * @date   06/20/2010 10:55:26 AM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>

#include "evbuffer.h"

inline void evbuffer_cb_fin(evbuffer *buf)
{
    if (buf->cbp->auto_free)
        free(buf->cbp->data);

    buf->cbp->data = NULL;
    buf->cbp->read = false;
    buf->cbp->bytes_expected = 0;
    buf->cbp = buf->cbp->next;
}

inline void evbuffer_read_fin(evbuffer *buf)
{
    buf->cbp->read = true;
    buf->readp = buf->readp->next;
}

inline evbuffer_node *evbuffer_getcb(evbuffer *buf)
{
    if (EVBUFFER_HASCB(buf))
        return NULL;
    return buf->cbp;
}

inline evbuffer_node *evbuffer_get(evbuffer *buf)
{
    if (EVBUFFER_EMPTY(buf))
        return NULL;
    return buf->readp;
}

inline int evbuffer_put(evbuffer *buf, char *msg, size_t msg_len, callback_p_t cb, bool auto_free)
{
    evbuffer_node *node = NULL;

    if (EVBUFFER_FULL(buf))
        return 0;

    node            = buf->writep;
    buf->writep     = buf->writep->next;
    node->data      = msg;
    node->length    = msg_len;
    node->cb        = cb;
    node->read      = false;
    node->auto_free = auto_free;
    node->offset    = 0;

    return 1;
}

void evbuffer_free(evbuffer *buf)
{
    evbuffer_node  *p = buf->writep;
    register size_t i;

    for (i = 0; i < buf->length; ++i) {
        if (p->data != NULL && p->auto_free)
            free(buf->writep->data);
        p = p->next;
    }
    free(buf->writep);
}

evbuffer *evbuffer_new(size_t init_len)
{
    evbuffer      *buf = NULL;
    evbuffer_node *nodes = NULL, *p = NULL;
    register size_t i;

    /* fix bad params */
    if (init_len <= 2)
        init_len = 2;

    if ( ( buf = (evbuffer *)malloc(sizeof(evbuffer)) ) == NULL )
        return NULL;

    if ( ( nodes = (evbuffer_node *)malloc(sizeof(evbuffer_node) * init_len ) ) == NULL )
        goto malloc_data_error;

    p = buf->cbp = buf->writep = buf->readp = nodes;

    for ( i = 1; i < init_len; ++i ) {
        p->data = NULL;
        p->read = false;
        p->bytes_expected = 0;
        p = p->next = ++nodes;
    }

    p->data = NULL;
    p->next     = buf->writep;
    buf->length = init_len;

    return buf;

malloc_data_error:
    free(buf);
    return NULL;
}
