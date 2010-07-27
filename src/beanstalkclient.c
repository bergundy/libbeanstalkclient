/**
 * =====================================================================================
 * @file   beanstalkclient.c
 * @brief  nonblocking implementation of a beanstalk client
 * @date   07/05/2010 07:01:09 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#ifdef __cplusplus
    extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/socket.h>
#include <sockutils.h>
#include "beanstalkclient.h"

#define cb_data_init(cb_data, on_success, on_error, user_data) do {     \
    cb_data->on_error   = on_error;                                     \
    cb_data->on_success = on_success;                                   \
    cb_data->user_data  = user_data;                                    \
} while (false)

#define CONST_STRLEN(str) (sizeof(str)/sizeof(char)-1)

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

bsc *bsc_new( const char *host, const char *port, error_callback_p_t onerror,
                  size_t buf_len, size_t vec_len, size_t vec_min, char **errorstr )
{
    bsc *client = NULL;

    if ( host == NULL || port == NULL )
        return NULL;

    if ( ( client = (bsc *)malloc(sizeof(bsc) ) ) == NULL )
        return NULL;

    client->host = client->port = NULL;
    client->vec = NULL;
    client->cbq = NULL;
    client->outq = NULL;

    if ( ( client->host = strdup(host) ) == NULL )
        goto host_strdup_err;

    if ( ( client->port = strdup(port) ) == NULL )
        goto port_strdup_err;

    if ( ( client->vec = ivector_new(vec_len) ) == NULL )
        goto ivector_new_err;

    if ( ( client->cbq = queue_new(buf_len) ) == NULL )
        goto evbuffer_new_err;

    if ( ( client->outq = ioq_new(buf_len) ) == NULL )
        goto ioq_new_err;

    client->vec_min     = vec_min;
    client->onerror     = onerror;

    if ( !bsc_connect(client, errorstr) )
        goto connect_error;

    return client;

connect_error:
    ioq_free(client->outq);
ioq_new_err:
    queue_free(client->cbq);
evbuffer_new_err:
    ivector_free(client->vec);
ivector_new_err:
    free(client->port);
port_strdup_err:
    free(client->host);
host_strdup_err:
    free(client);
    if (errorstr != NULL && *errorstr == NULL)
        *errorstr = strdup("out of memory");
    return NULL;
}

void bsc_free(bsc *client)
{
    free(client->host);
    free(client->port);
    ioq_free(client->outq);
    queue_free(client->cbq);
    ivector_free(client->vec);
    free(client);
}

bool bsc_connect(bsc *client, char **errorstr)
{
    ptrdiff_t queue_diff;

    if ( ( client->fd = tcp_client(client->host, client->port, NONBLK | REUSE, errorstr) ) == SOCKERR )
        return false;

    if ( ( queue_diff = (client->cbq->used - IOQ_NODES_USED(client->outq) ) ) > 0 ) {
        client->outq->output_p -= queue_diff;
        if (client->outq->output_p < client->outq->nodes_begin)
            client->outq->output_p += client->outq->nodes_end - client->outq->nodes_begin + 1;
    }
    client->vec->som = client->vec->eom = client->vec->data;

    return true;
}

void bsc_disconnect(bsc *client)
{
    while ( close(client->fd) == SOCKERR && errno != EBADF ) ;
}

bool bsc_reconnect(bsc *client, char **errorstr)
{
    bsc_disconnect(client);
    return bsc_connect(client, errorstr);
}

static void sock_write_error(void *self)
{
    bsc *client = (bsc *)self;
    switch (errno) {
        case EAGAIN:
        case EINTR:
            /* temporary error, the callback will be rescheduled */
            break;
        case EINVAL:
        default:
            /* unexpected socket error - yield client callback */
            client->onerror(client, BSC_ERROR_SOCKET);
    }
}

void bsc_write(bsc *client)
{
    ioq_write_nv(client->outq, client->fd, sock_write_error, (void *)client);

    return;
}

void bsc_read(bsc *client)
{
    /* variable declaration / initialization */
    ivector *vec = client->vec;
    queue    *buf = client->cbq;
    queue_node *node = NULL;
    char    ctmp, *eom  = NULL;
    ssize_t bytes_recv, bytes_processed = 0;

    /* expand vector on demand */
    if (IVECTOR_FREE(vec) < client->vec_min && !ivector_expand(vec))
        /* temporary (out of memory) error, the callback will be rescheduled */
        return;

    /* recieve data */
    if ( ( bytes_recv = recv(client->fd, vec->eom, IVECTOR_FREE(vec), 0) ) < 1 ) {
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
                client->onerror(client, BSC_ERROR_SOCKET);
                return;
        }
    }

    //printf("recv: '%s'\n", vec->eom);
    while (bytes_processed != bytes_recv) {
        if ( (node = AQUEUE_REAR(buf) ) == NULL ) {
            /* critical error */
            client->onerror(client, BSC_ERROR_INTERNAL);
            return;
        }
        if (node->bytes_expected) {
            if ( bytes_recv - bytes_processed < node->bytes_expected + 2 - (vec->eom-vec->som) )
                goto in_middle_of_msg;

            eom = vec->som + node->bytes_expected;
            bytes_processed += eom - vec->eom + 2;
            if (node->cb != NULL) {
                *eom = '\0';
                node->cb(client, node, vec->som, eom - vec->som);
            }
            vec->eom = vec->som = eom + 2;
            QUEUE_FIN_CMD(buf);
        }
        else {
            if ( ( eom = (char *)memchr(vec->eom, '\n', bytes_recv - bytes_processed) ) == NULL )
                goto in_middle_of_msg;

            bytes_processed += ++eom - vec->eom;
            if (node->cb != NULL) {
                ctmp = *eom;
                *eom = '\0';
                node->cb(client, node, vec->som, eom - vec->som);
                *eom = ctmp;
            }
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

bsc_error_t bsc_put(bsc        *client,
                    bsc_cb_p_t  user_cb,
                    void       *user_data,
                    uint32_t    priority,
                    uint32_t    delay,
                    uint32_t    ttr,
                    size_t      bytes,
                    const char *data)
{
    if ( IOQ_NODES_FREE(client->outq) < 3)
        return BSC_ERROR_QUEUE_FULL;

    if ( ( AQUEUE_FRONT_NV((client)->cbq)->data = bsp_gen_put_hdr(
        &(AQUEUE_FRONT_NV((client)->cbq)->len), priority, delay, ttr, bytes) ) == NULL )
        return BSC_ERROR_MEMORY;

    IOQ_PUT_NV( (client)->outq, 
        AQUEUE_FRONT_NV((client)->cbq)->data, AQUEUE_FRONT_NV((client)->cbq)->len, false );

    IOQ_PUT_NV( (client)->outq, (char *)data, bytes, false );

    IOQ_PUT_NV( (client)->outq, CRLF, CONST_STRLEN(CRLF), false );

    AQUEUE_FRONT_NV((client)->cbq)->cb              = user_cb;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data         = user_data;
    AQUEUE_FRONT_NV((client)->cbq)->bytes_expected  = 0;
    AQUEUE_FRONT_NV((client)->cbq)->is_allocated    = true;
    AQUEUE_FIN_PUT((client)->cbq);

    return BSC_ERROR_NONE;
}

#ifdef __cplusplus
    }
#endif
