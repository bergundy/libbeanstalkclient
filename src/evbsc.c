/**
 * =====================================================================================
 * @file   evbsc.c
 * @brief  
 * @date   07/05/2010 07:01:09 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sockutils.h>
#include "evbsc.h"

static void write_ready(EV_P_ ev_io *w, int revents);
static void read_ready(EV_P_ ev_io *w, int revents);

static queue *queue_new(size_t size)
{
    queue *q;
    if ( ( q = (queue *)malloc(sizeof(queue)) ) == NULL )
        return NULL;

    if ( ( q->nodes = (queue_node *)malloc(sizeof(queue_node) * size) ) == NULL )
        goto node_malloc_error;

    q->size = size;
    q->rear = q->front = 0;
    q->used = 0;

    return q;

node_malloc_error:
    free(q);
    return NULL;
}

static void queue_free(queue *q)
{
    while ( !AQUEUE_EMPTY(q) )
        QUEUE_FIN_CMD(q);
    free(q);
}

evbsc *evbsc_new( struct ev_loop *loop, char *host, char *port, unsigned reconnect_attempts,
                  size_t buf_len, size_t vec_len, size_t vec_rlen, size_t vec_min, char **errorstr )
{
    evbsc *bsc = NULL;

    if ( host == NULL || port == NULL )
        return NULL;

    if ( ( bsc = (evbsc *)malloc(sizeof(evbsc) ) ) == NULL )
        return NULL;

    bsc->host = bsc->port = NULL;
    bsc->vec = NULL;
    bsc->cbq = NULL;
    bsc->outq = NULL;

    if ( ( bsc->host = strdup(host) ) == NULL )
        goto host_strdup_err;

    if ( ( bsc->port = strdup(port) ) == NULL )
        goto port_strdup_err;

    if ( ( bsc->vec = evvector_new(vec_len, vec_rlen) ) == NULL )
        goto evvector_new_err;

    if ( ( bsc->cbq = queue_new(buf_len) ) == NULL )
        goto evbuffer_new_err;

    if ( ( bsc->outq = ioq_new(buf_len) ) == NULL )
        goto ioq_new_err;

    bsc->reconnect_attempts = reconnect_attempts;
    bsc->vec_min            = vec_min;

    if ( !evbsc_connect(bsc, loop, errorstr) )
        goto connect_error;

    return bsc;

connect_error:
    ioq_free(bsc->outq);
ioq_new_err:
    queue_free(bsc->cbq);
evbuffer_new_err:
    evvector_free(bsc->vec);
evvector_new_err:
    free(bsc->port);
port_strdup_err:
    free(bsc->host);
host_strdup_err:
    if (errorstr != NULL && *errorstr != NULL)
        *errorstr = strdup("out of memory");

    free(bsc);
    return NULL;
}

void evbsc_free(evbsc *bsc)
{
    free(bsc->host);
    free(bsc->port);
    ioq_free(bsc->outq);
    queue_free(bsc->cbq);
    evvector_free(bsc->vec);
    free(bsc);
}

bool evbsc_connect(evbsc *bsc, struct ev_loop *loop, char **errorstr)
{
    unsigned i;
    for ( i = 0; i < bsc->reconnect_attempts; ++i)
        if ( ( bsc->fd = tcp_client(bsc->host, bsc->port, NONBLK | REUSE, errorstr) ) != SOCKERR )
            goto connect_success;

    return false;

connect_success:
    ev_io_init( &(bsc->rw), read_ready,  bsc->fd, EV_READ  );
    ev_io_init( &(bsc->ww), write_ready, bsc->fd, EV_WRITE );
    bsc->rw.data = bsc;
    bsc->ww.data = bsc;
    ev_io_start(loop, &(bsc->rw));
    if (!IOQ_EMPTY(bsc->outq))
        ev_io_start(loop, &(bsc->ww));

    return true;
}

void evbsc_disconnect(evbsc *bsc, struct ev_loop *loop)
{
    ev_io_stop(loop, &(bsc->rw));
    ev_io_stop(loop, &(bsc->ww));
    while ( close(bsc->fd) == SOCKERR && errno != EBADF ) ;
}

bool evbsc_reconnect(evbsc *bsc, struct ev_loop *loop)
{
    evbsc_disconnect(bsc, loop);
    return evbsc_connect(bsc, loop, NULL);
}

static void write_ready(EV_P_ ev_io *w, int revents)
{
    printf("write ready\n");
    evbsc *bsc = (evbsc *)w->data;
    IOQ_WRITE_NV(bsc->outq, w->fd, sockerror);
    if (IOQ_EMPTY(bsc->outq))
        ev_io_stop(loop, &(bsc->ww));

    return;
        
sockerror:
    switch (errno) {
        case EAGAIN:
        case EINTR:
            /* temporary error, the callback will be rescheduled */
            break;
        case EINVAL:
        default:
            /* unexpected socket error - reconnect */
            evbsc_reconnect(bsc, loop);
    }
}

static void read_ready(EV_P_ ev_io *w, int revents)
{
    printf("read ready\n");
    /* variable declaration / initialization */
    evbsc    *bsc = (evbsc *)w->data;
    evvector *vec = bsc->vec;
    queue    *buf = bsc->cbq;
    queue_node *node = NULL;
    char    ctmp, *eom  = NULL;
    ssize_t bytes_recv, bytes_processed = 0;

    /* expand vector on demand */
    if (EVVECTOR_FREE(vec) < bsc->vec_min && !evvector_expand(vec))
        /* temporary (out of memory) error, the callback will be rescheduled */
        return;

    /* recieve data */
    if ( ( bytes_recv = recv(bsc->fd, vec->eom, EVVECTOR_FREE(vec), 0) ) < 1 ) {
        switch (bytes_recv) {
            case SOCKERR:
                switch (errno) {
                    case EAGAIN:
                    case EINTR:
                        /* temporary error, the callback will be rescheduled */
                        return;
                }
            default:
                /* unexpected socket error - reconnect */
                evbsc_reconnect(bsc, loop);
                return;
        }
    }

    //printf("recv: '%s'\n", vec->eom);
    while (bytes_processed != bytes_recv) {
        printf("proccessed: %d/%d/%d\n", bytes_processed, bytes_recv, vec->size);
        if ( (node = AQUEUE_REAR(buf) ) == NULL ) {
            /* critical error */
            evbsc_disconnect(bsc, loop);
            return;
        }
        if (node->bytes_expected) {
            if ( bytes_recv - bytes_processed < node->bytes_expected + 2 - (vec->eom-vec->som) )
                goto in_middle_of_msg;

            eom = vec->som + node->bytes_expected;
            bytes_processed += eom - vec->eom + 2;
            *eom = '\0';
            node->cb(node, vec->som);
            vec->eom = vec->som = eom + 2;
            QUEUE_FIN_CMD(buf);
        }
        else {
            if ( ( eom = memchr(vec->eom, '\n', bytes_recv - bytes_processed) ) == NULL )
                goto in_middle_of_msg;

            bytes_processed += ++eom - vec->eom;
            ctmp = *eom;
            *eom = '\0';
            node->cb(node, vec->som);
            *eom = ctmp;
            vec->eom = vec->som = eom;
            if (!node->bytes_expected)
                QUEUE_FIN_CMD(buf);
        }
    }
    vec->eom = vec->som = vec->data;
    return;

in_middle_of_msg:
    vec->eom += bytes_recv - bytes_processed;
}
