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

#define CONST_STRLEN(str) (sizeof(str)/sizeof(char)-1)

#define ENQ_CMD_(client, gen_cmd, nodes, u_cb, ...) (                                           \
    AQUEUE_NODES_FREE((client)->outq) - (client)->outq_offset < (nodes) ? BSC_ERROR_QUEUE_FULL  \
    : ( ( AQUEUE_FRONT_NV((client)->cbq)->data                                                  \
        = gen_cmd( &(AQUEUE_FRONT_NV((client)->cbq)->len),                                      \
            &(AQUEUE_FRONT_NV((client)->cbq)->is_allocated), ## __VA_ARGS__) ) == NULL          \
        ? BSC_ERROR_MEMORY                                                                      \
        : ( IOQ_PUT_NV( (client)->outq, AQUEUE_FRONT_NV((client)->cbq)->data,                   \
                    AQUEUE_FRONT_NV((client)->cbq)->len, false ),                               \
              AQUEUE_FRONT_NV((client)->cbq)->cb             = u_cb,                            \
              AQUEUE_FRONT_NV((client)->cbq)->bytes_expected = 0,                               \
              AQUEUE_FRONT_NV((client)->cbq)->outq_offset    = (nodes) - 1,                     \
              BSC_ERROR_NONE ) ) )

#define ENQ_CMD(client, cmd, u_cb, ...) ENQ_CMD_(client, bsp_gen_ ## cmd ## _cmd, 1, u_cb, ## __VA_ARGS__)

static void got_put_res(bsc *client, queue_node *node, void *data, size_t len);
static void got_reserve_res(bsc *client, queue_node *node, void *data, size_t len);
static void got_delete_res(bsc *client, queue_node *node, void *data, size_t len);

static queue *queue_new(size_t size)
{
    queue *q;
    union bsc_cmd_info *cmd_info;
    register size_t i;

    if ( ( q = (queue *)malloc(sizeof(queue)) ) == NULL )
        return NULL;

    if ( ( q->nodes = (queue_node *)malloc(sizeof(queue_node) * size) ) == NULL )
        goto node_malloc_error;

    if ( ( cmd_info = (union bsc_cmd_info *)malloc(sizeof(union bsc_cmd_info) * size) ) == NULL )
        goto cmd_info_malloc_error;

    for (i = 0; i < size; ++i)
        q->nodes[i].cb_data = cmd_info+i;

    q->size = size;
    q->rear = q->front = 0;
    q->used = 0;

    return q;

cmd_info_malloc_error:
    free(q->nodes);
node_malloc_error:
    free(q);
    return NULL;
}

static void queue_free(queue *q)
{
    while ( !AQUEUE_EMPTY(q) )
        QUEUE_FIN_CMD(q);
    free(q->nodes[0].cb_data);
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
    client->outq_offset = 0;

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

    if ( ( queue_diff = client->outq_offset + (client->cbq->used - client->outq->used ) ) > 0 ) {
        client->outq->rear -= queue_diff;
        client->outq->used += queue_diff;
        if (client->outq->rear < 0)
            client->outq->rear += client->outq->size;
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

void bsc_write(bsc *client)
{
    queue_node *node = NULL;
    size_t  i;
    ssize_t nodes_written = ioq_write_nv(client->outq, client->fd);

    if (nodes_written < 0)
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
    else
        for (i = 0; nodes_written > 0; ++i) {
            node = client->cbq->nodes + ((client->cbq->rear + i) % (client->cbq->size - 1));
            client->outq_offset += node->outq_offset;
            nodes_written -= node->outq_offset + 1;
        }
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

bsc_error_t bsc_put(bsc            *client,
                    bsc_put_user_cb user_cb,
                    void           *user_data,
                    uint32_t        priority,
                    uint32_t        delay,
                    uint32_t        ttr,
                    size_t          bytes,
                    const char     *data,
                    bool            free_when_finished)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD_(client, bsp_gen_put_hdr, 3, got_put_res, priority, delay, ttr, bytes) ) != BSC_ERROR_NONE )
        return error;

    IOQ_PUT_NV( (client)->outq, (char *)data, bytes, false );
    IOQ_PUT_NV( (client)->outq, CRLF, CONST_STRLEN(CRLF), false );

    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.user_data         = user_data;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.user_cb           = user_cb;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.request.priority  = priority;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.request.ttr       = ttr;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.request.delay     = delay;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.request.bytes     = bytes;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.request.data      = data;
    AQUEUE_FRONT_NV((client)->cbq)->cb_data->put_info.request.autofree  = free_when_finished;

    QUEUE_FIN_PUT((client)->cbq);

    return BSC_ERROR_NONE;
}

static void got_put_res(bsc *client, queue_node *node, void *data, size_t len)
{
    struct bsc_put_info *put_info = &(node->cb_data->put_info);

    client->outq_offset -= 2;

    put_info->response.code = bsp_get_put_res(data, &(put_info->response.id));

    if (put_info->user_cb != NULL)
        put_info->user_cb(client, put_info);
    if (put_info->request.autofree)
        free((char *)put_info->request.data);
}

bsc_error_t bsc_reserve(bsc                *client,
                        bsc_reserve_user_cb user_cb,
                        void               *user_data,
                        int32_t             timeout)
{
    bsc_error_t error;

    if (timeout < 0) {
        if ( ( error = ENQ_CMD(client, reserve, got_reserve_res) ) != BSC_ERROR_NONE )
            return error;
    }
    else {
        if ( ( error = ENQ_CMD(client, reserve_with_to, got_reserve_res, timeout) ) != BSC_ERROR_NONE )
            return error;
    }

    AQUEUE_FRONT_NV(client->cbq)->cb_data->reserve_info.request.timeout   = timeout;
    AQUEUE_FRONT_NV(client->cbq)->cb_data->reserve_info.user_data         = user_data;
    AQUEUE_FRONT_NV(client->cbq)->cb_data->reserve_info.user_cb           = user_cb;
    QUEUE_FIN_PUT((client)->cbq);
    return BSC_ERROR_NONE;
}

static void got_reserve_res(bsc *client, queue_node *node, void *data, size_t len)
{
    struct bsc_reserve_info *reserve_info = &(node->cb_data->reserve_info);

    if (node->bytes_expected) {
        reserve_info->response.data = data;
        if (reserve_info->user_cb != NULL)
            reserve_info->user_cb(client, reserve_info);
    }
    else {
        reserve_info->response.code = bsp_get_reserve_res(data, &(reserve_info->response.id), &(reserve_info->response.bytes));
        if (reserve_info->response.code == BSP_RESERVE_RES_RESERVED)
            node->bytes_expected = reserve_info->response.bytes;
        else {
            if (reserve_info->user_cb != NULL)
                reserve_info->user_cb(client, reserve_info);
        }
    }
}

bsc_error_t bsc_delete(bsc                *client,
                       bsc_delete_user_cb user_cb,
                       void               *user_data,
                       uint64_t            id)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, delete, got_delete_res, id) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbq)->cb_data->delete_info.request.id   = id;
    AQUEUE_FRONT_NV(client->cbq)->cb_data->delete_info.user_data    = user_data;
    AQUEUE_FRONT_NV(client->cbq)->cb_data->delete_info.user_cb      = user_cb;
    QUEUE_FIN_PUT((client)->cbq);
    return BSC_ERROR_NONE;
}

static void got_delete_res(bsc *client, queue_node *node, void *data, size_t len)
{
    if (node->cb_data->delete_info.user_cb != NULL) {
        node->cb_data->delete_info.response.code = bsp_get_delete_res(data);
        node->cb_data->delete_info.user_cb(client, &(node->cb_data->delete_info));
    }
}

#ifdef __cplusplus
    }
#endif
