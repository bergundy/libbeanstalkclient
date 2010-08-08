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
#include <beanstalkproto.h>
#include "beanstalkclient.h"

#define CONST_STRLEN(str) (sizeof(str)/sizeof(char)-1)

#define ENQ_CMD_(client, gen_cmd, nodes, u_cb, ...) (                                             \
    BSC_BUFFER_NODES_FREE(client) < (nodes) ? BSC_ERROR_QUEUE_FULL                                \
    : ( ( AQUEUE_FRONT_NV((client)->cbqueue)->data                                                \
        = gen_cmd( &(AQUEUE_FRONT_NV((client)->cbqueue)->len),                                    \
            &(AQUEUE_FRONT_NV((client)->cbqueue)->is_allocated), ## __VA_ARGS__) ) == NULL        \
        ? BSC_ERROR_MEMORY                                                                        \
        : ( IOQ_PUT_NV( (client)->outq, AQUEUE_FRONT_NV((client)->cbqueue)->data,                 \
                    AQUEUE_FRONT_NV((client)->cbqueue)->len, false ),                             \
              AQUEUE_FRONT_NV((client)->cbqueue)->cb             = u_cb,                          \
              AQUEUE_FRONT_NV((client)->cbqueue)->bytes_expected = 0,                             \
              AQUEUE_FRONT_NV((client)->cbqueue)->outq_offset    = (nodes) - 1,                   \
              BSC_ERROR_NONE ) ) )

#define ENQ_CMD(client, cmd, u_cb, ...) ENQ_CMD_(client, bsp_gen_ ## cmd ## _cmd, 1, u_cb, ## __VA_ARGS__)

#define GENERIC_RES_FUNC(cmd_type) \
static void got_ ## cmd_type ## _res(bsc *client, cbq_node *node, const char *data, size_t len)     \
{                                                                                                   \
    if (node->cb_data->cmd_type ## _info.user_cb != NULL) {                                         \
        node->cb_data->cmd_type ## _info.response.code = bsp_get_ ## cmd_type ## _res(data);        \
        node->cb_data->cmd_type ## _info.user_cb(client, &(node->cb_data->cmd_type ## _info));      \
    }                                                                                               \
}

static bool insert_tube_before(const char *name, struct bsc_tube_list **l);
static bool insert_tube_after(const char *name, struct bsc_tube_list *l);

static void got_put_res(bsc *client, cbq_node *node, const char *data, size_t len);
static void got_use_res(bsc *client, cbq_node *node, const char *data, size_t len);
static void got_reserve_res(bsc *client, cbq_node *node, const char *data, size_t len);
GENERIC_RES_FUNC(delete)
GENERIC_RES_FUNC(release)
GENERIC_RES_FUNC(bury)
GENERIC_RES_FUNC(touch)
static void got_watch_res(bsc *client, cbq_node *node, const char *data, size_t len);
static void got_ignore_res(bsc *client, cbq_node *node, const char *data, size_t len);
static void got_peek_res(bsc *client, cbq_node *node, const char *data, size_t len);

static cbq *cbq_new(size_t size)
{
    cbq *q;
    union bsc_cmd_info *cmd_info;
    register size_t i;

    if ( ( q = (cbq *)malloc(sizeof(cbq)) ) == NULL )
        return NULL;

    if ( ( q->nodes = (cbq_node *)malloc(sizeof(cbq_node) * size) ) == NULL )
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

static void cbq_free(cbq *q)
{
    while ( !AQUEUE_EMPTY(q) )
        QUEUE_FIN_CMD(q);
    free(q->nodes[0].cb_data);
    free(q);
}

bsc *bsc_new(const char *host, const char *port, const char *default_tube,
             error_callback_p_t onerror, size_t buf_len,
             size_t vec_len, size_t vec_min, char **errorstr)
{
    bsc *client = NULL;

    if ( host == NULL || port == NULL )
        return NULL;

    if ( ( client = (bsc *)malloc(sizeof(bsc) ) ) == NULL )
        return NULL;

    client->buffer_fill_cb = NULL;
    client->host = client->port = NULL;
    client->vec = NULL;
    client->tubecbq = client->cbqueue = NULL;
    client->tubeq = client->outq = NULL;
    client->default_tube = NULL;
    client->watched_tubes = NULL;

    if ( ( client->host = strdup(host) ) == NULL )
        goto host_strdup_err;

    if ( ( client->port = strdup(port) ) == NULL )
        goto port_strdup_err;

    if ( ( client->vec = ivector_new(vec_len) ) == NULL )
        goto ivector_new_err;

    if ( ( client->cbqueue = cbq_new(buf_len) ) == NULL )
        goto evbuffer_new_err;

    if ( ( client->outq = ioq_new(buf_len) ) == NULL )
        goto ioq_new_err;

    if ( ( client->default_tube = strdup(default_tube) ) == NULL )
        goto tube_dup_default_err;

    if ( ( client->watched_tubes = (struct bsc_tube_list *)malloc(sizeof(struct bsc_tube_list)) ) == NULL )
        goto tube_list_malloc_err;

    if ( ( client->watched_tubes->name = strdup(default_tube) ) == NULL )
        goto tube_dup_list_err;

    client->watched_tubes->next = NULL;

    client->vec_min     = vec_min;
    client->onerror     = onerror;
    client->outq_offset = 0;
    client->watched_tubes_count = 1;

    if ( !bsc_connect(client, errorstr) )
        goto connect_err;

    return client;

connect_err:
    free(client->watched_tubes->name);
tube_dup_list_err:
    free(client->watched_tubes);
tube_list_malloc_err:
    free(client->default_tube);
tube_dup_default_err:
    ioq_free(client->outq);
ioq_new_err:
    cbq_free(client->cbqueue);
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
    struct bsc_tube_list *p1 = client->watched_tubes, *p2 = NULL;

    do {
        p2 = p1->next;
        free(p1->name);
        free(p1);
        p1 = p2;
    } while (p1 != NULL);

    free(client->default_tube);
    free(client->host);
    free(client->port);
    ioq_free(client->outq);
    cbq_free(client->cbqueue);
    ivector_free(client->vec);
    free(client);
}

bool bsc_connect(bsc *client, char **errorstr)
{
    ptrdiff_t queue_diff;
    bool      check_default    = true, ignore_default = true;
    struct    bsc_tube_list *p = NULL;
    ioq      *tmpq             = NULL;
    cbq      *tmpcbq           = NULL;
    int       cmp_res;

    if ( ( client->fd = tcp_client(client->host, client->port, NONBLK | REUSE, errorstr) ) == SOCKERR )
        return false;

    if ( ( queue_diff = client->outq_offset + (client->cbqueue->used - client->outq->used ) ) > 0 ) {
        client->outq->rear -= queue_diff;
        client->outq->used += queue_diff;
        if (client->outq->rear < 0)
            client->outq->rear += client->outq->size;
    }
    client->vec->som = client->vec->eom = client->vec->data;

    if (client->tubeq != NULL)
        ioq_free(client->tubeq);
    tmpq = client->outq;
    if ( ( client->outq = ioq_new(client->watched_tubes_count + 2) ) == NULL )
        goto out_of_memory;

    if (client->tubecbq != NULL)
        cbq_free(client->tubecbq);
    tmpcbq = client->cbqueue;
    if ( ( client->cbqueue = cbq_new(client->watched_tubes_count + 2) ) == NULL )
        goto out_of_memory;

    if ( ( strcmp(client->default_tube, BSC_DEFAULT_TUBE) ) != 0 )
        if ( bsc_use(client, NULL, NULL, client->default_tube) != BSC_ERROR_NONE )
            goto out_of_memory;

    for (p = client->watched_tubes; p != NULL; p = p->next) {
        if ( check_default && ( cmp_res = strcmp(p->name, BSC_DEFAULT_TUBE) ) >= 0 ) {
            check_default = false;
            if (cmp_res == 0)
                ignore_default = false;
        }
        else if ( bsc_watch(client, NULL, NULL, p->name) != BSC_ERROR_NONE )
            goto out_of_memory;
    }

    if (ignore_default && bsc_ignore(client, NULL, NULL, BSC_DEFAULT_TUBE) != BSC_ERROR_NONE)
        goto out_of_memory;

    client->tubeq   = client->outq;
    client->outq    = tmpq;
    client->tubecbq = client->cbqueue;
    client->cbqueue = tmpcbq;

    return true;

out_of_memory:
    if (errorstr != NULL)
        *errorstr = strdup("out of memory");
    return false; 
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
    cbq_node *node = NULL;
    size_t  i;
    ssize_t nodes_written;
    if (client->tubeq != NULL) {
        nodes_written = ioq_write_nv(client->tubeq, client->fd);
        if (AQUEUE_EMPTY(client->tubeq)) {
            ioq_free(client->tubeq);
            client->tubeq = NULL;
        }
    }
    else
        nodes_written = ioq_write_nv(client->outq, client->fd);

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
            node = client->cbqueue->nodes + ((client->cbqueue->rear + i) % (client->cbqueue->size - 1));
            client->outq_offset += node->outq_offset;
            nodes_written -= node->outq_offset + 1;
        }
}

void bsc_read(bsc *client)
{
    /* variable declaration / initialization */
    ivector  *vec  = client->vec;
    cbq      *buf  = client->cbqueue;
    cbq_node *node = NULL;
    char      ctmp, *eom  = NULL;
    ssize_t   bytes_recv, bytes_processed = 0;

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
        if (client->tubecbq != NULL) {
            if (AQUEUE_EMPTY(client->tubecbq)) {
                cbq_free(client->tubecbq);
                client->tubecbq = NULL;
                buf = client->cbqueue;
            }
            else
                buf = client->tubecbq;
        }
                
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

    IOQ_PUT_NV( client->outq, (char *)data, bytes, false );
    IOQ_PUT_NV( client->outq, (char *)CRLF, CONST_STRLEN(CRLF), false );

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.user_data         = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.user_cb           = user_cb;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.request.priority  = priority;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.request.ttr       = ttr;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.request.delay     = delay;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.request.bytes     = bytes;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.request.data      = data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->put_info.request.autofree  = free_when_finished;

    QUEUE_FIN_PUT(client);

    return BSC_ERROR_NONE;
}

static void got_put_res(bsc *client, cbq_node *node, const char *data, size_t len)
{
    struct bsc_put_info *put_info = &(node->cb_data->put_info);

    client->outq_offset -= 2;

    put_info->response.code = bsp_get_put_res(data, &(put_info->response.id));

    if (put_info->user_cb != NULL)
        put_info->user_cb(client, put_info);
    if (put_info->request.autofree)
        free((char *)put_info->request.data);
}

bsc_error_t bsc_use(bsc                *client,
                    bsc_use_user_cb     user_cb,
                    void               *user_data,
                    const char         *tube)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, use, got_use_res, tube) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->use_info.request.tube = tube;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->use_info.user_data    = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->use_info.user_cb      = user_cb;
    QUEUE_FIN_PUT(client);

    return BSC_ERROR_NONE;
}

static void got_use_res(bsc *client, cbq_node *node, const char *data, size_t len)
{
    struct bsc_use_info *use_info = &(node->cb_data->use_info);

    use_info->response.code = bsp_get_use_res(data, &(use_info->response.tube));

    if ( strcmp(client->default_tube, use_info->response.tube ) != 0 ) {
        free(client->default_tube);
        if ( ( client->default_tube = strdup(use_info->response.tube) ) == NULL )
            client->onerror(client, BSC_ERROR_MEMORY);
    }
    if (use_info->user_cb != NULL)
        use_info->user_cb(client, use_info);
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

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->reserve_info.request.timeout   = timeout;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->reserve_info.user_data         = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->reserve_info.user_cb           = user_cb;
    QUEUE_FIN_PUT(client);
    return BSC_ERROR_NONE;
}

static void got_reserve_res(bsc *client, cbq_node *node, const char *data, size_t len)
{
    struct bsc_reserve_info *reserve_info = &(node->cb_data->reserve_info);

    if (node->bytes_expected) {
        reserve_info->response.data = (void *)data;
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
                       bsc_delete_user_cb  user_cb,
                       void               *user_data,
                       uint64_t            id)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, delete, got_delete_res, id) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->delete_info.request.id   = id;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->delete_info.user_data    = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->delete_info.user_cb      = user_cb;
    QUEUE_FIN_PUT(client);
    return BSC_ERROR_NONE;
}

bsc_error_t bsc_release(bsc                 *client,
                        bsc_release_user_cb  user_cb,
                        void                *user_data,
                        uint64_t             id, 
                        uint32_t             priority,
                        uint32_t             delay)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, release, got_release_res, id, priority, delay) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->release_info.request.id         = id;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->release_info.request.priority   = priority;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->release_info.request.delay      = delay;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->release_info.user_data          = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->release_info.user_cb            = user_cb;
    QUEUE_FIN_PUT(client);
    return BSC_ERROR_NONE;
}

bsc_error_t bsc_bury(bsc                *client,
                     bsc_bury_user_cb    user_cb,
                     void               *user_data,
                     uint64_t            id,
                     uint32_t            priority)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, bury, got_bury_res, id, priority) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->bury_info.request.id         = id;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->bury_info.request.priority   = priority;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->bury_info.user_data          = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->bury_info.user_cb            = user_cb;
    QUEUE_FIN_PUT(client);
    return BSC_ERROR_NONE;
}

bsc_error_t bsc_touch(bsc                *client,
                      bsc_touch_user_cb   user_cb,
                      void               *user_data,
                      uint64_t            id)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, touch, got_touch_res, id) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->touch_info.request.id   = id;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->touch_info.user_data    = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->touch_info.user_cb      = user_cb;
    QUEUE_FIN_PUT(client);
    return BSC_ERROR_NONE;
}

bsc_error_t bsc_watch(bsc                *client,
                      bsc_watch_user_cb   user_cb,
                      void               *user_data,
                      const char         *tube)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, watch, got_watch_res, tube) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->watch_info.request.tube = tube;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->watch_info.user_data    = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->watch_info.user_cb      = user_cb;
    QUEUE_FIN_PUT(client);

    return BSC_ERROR_NONE;
}

static void got_watch_res(bsc *client, cbq_node *node, const char *data, size_t len)
{
    struct bsc_watch_info *watch_info = &(node->cb_data->watch_info);
    struct bsc_tube_list *p = client->watched_tubes, *prev = NULL;
    int cmp_res;
    bool matched_tube = false;

    watch_info->response.code = bsp_get_watch_res(data, &(watch_info->response.count));

    while (!matched_tube) {
        if ( ( cmp_res = strcmp(p->name, watch_info->request.tube) ) == 0 ) {
#ifdef DEBUG
            printf("cmp_res = 0: '%s' - '%s'\n", p->name, watch_info->request.tube);
#endif
            matched_tube = true;
        }
        else if (cmp_res > 0) {
            matched_tube = true;
            if (prev == NULL) {
#ifdef DEBUG
                printf("cmp_res > 0 (prev == NULL): '%s' - '%s'\n", p->name, watch_info->request.tube);
#endif
                if (!insert_tube_before(watch_info->request.tube, &client->watched_tubes))
                    client->onerror(client, BSC_ERROR_MEMORY);
            }
            else {
#ifdef DEBUG
                printf("cmp_res > 0 (prev != NULL): '%s' - '%s'\n", p->name, watch_info->request.tube);
#endif
                if (!insert_tube_after(watch_info->request.tube, prev))
                    client->onerror(client, BSC_ERROR_MEMORY);
            }
        }
        else if (p->next == NULL) {
            matched_tube = true;
#ifdef DEBUG
            printf("cmp_res < 0: '%s' - '%s'\n", p->name, watch_info->request.tube);
#endif
            if (!insert_tube_after(watch_info->request.tube, p))
                client->onerror(client, BSC_ERROR_MEMORY);
        }
#ifdef DEBUG
        else {
            printf("cmp_res < 0 (not inserted): '%s' - '%s'\n", p->name, watch_info->request.tube);
        }
#endif
        prev = p;
        p = p->next;
    }

    client->watched_tubes_count = watch_info->response.count;

    if (watch_info->user_cb != NULL)
        watch_info->user_cb(client, watch_info);
}

bsc_error_t bsc_ignore(bsc                *client,
                       bsc_ignore_user_cb  user_cb,
                       void               *user_data,
                       const char         *tube)
{
    bsc_error_t error;

    if ( ( error = ENQ_CMD(client, ignore, got_ignore_res, tube) ) != BSC_ERROR_NONE )
        return error;

    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->ignore_info.request.tube = tube;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->ignore_info.user_data    = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->ignore_info.user_cb      = user_cb;
    QUEUE_FIN_PUT(client);

    return BSC_ERROR_NONE;
}

static void got_ignore_res(bsc *client, cbq_node *node, const char *data, size_t len)
{
    struct bsc_ignore_info *ignore_info = &(node->cb_data->ignore_info);
    struct bsc_tube_list *p = client->watched_tubes, *prev = NULL;
    int cmp_res;

    ignore_info->response.code = bsp_get_ignore_res(data, &(ignore_info->response.count));

    if (ignore_info->response.code == BSP_RES_WATCHING) {
        while (true) {
            if ( ( cmp_res = strcmp(p->name, ignore_info->request.tube) ) == 0 ) {
                if (prev != NULL)
                    prev->next = p->next;
                else
                    client->watched_tubes = p->next;
                free(p->name);
                free(p);
                break;
            }
            else if (cmp_res > 0)
                break;
            else if (p->next == NULL)
                break;
            else {
                prev = p;
                p = p->next;
            }
        }

        client->watched_tubes_count = ignore_info->response.count;
    }

    if (ignore_info->user_cb != NULL)
        ignore_info->user_cb(client, ignore_info);
}

bsc_error_t bsc_peek(bsc                *client,
                     bsc_peek_user_cb    user_cb,
                     void               *user_data,
                     enum peek_cmd_e     peek_type,
                     uint64_t            id)
{
    bsc_error_t error;

    switch (peek_type) {
        case ID:
            if ( ( error = ENQ_CMD(client, peek, got_peek_res, id) ) != BSC_ERROR_NONE )
                return error;
        case READY:
            if ( ( error = ENQ_CMD(client, peek_ready, got_peek_res) ) != BSC_ERROR_NONE )
                return error;
        case DELAYED:
            if ( ( error = ENQ_CMD(client, peek_delayed, got_peek_res) ) != BSC_ERROR_NONE )
                return error;
        case BURIED:
            if ( ( error = ENQ_CMD(client, peek_buried, got_peek_res) ) != BSC_ERROR_NONE )
                return error;
    }
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->peek_info.user_data         = user_data;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->peek_info.user_cb           = user_cb;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->peek_info.request.peek_type = peek_type;
    AQUEUE_FRONT_NV(client->cbqueue)->cb_data->peek_info.request.id        = id;
    QUEUE_FIN_PUT(client);
    return BSC_ERROR_NONE;
}

static void got_peek_res(bsc *client, cbq_node *node, const char *data, size_t len)
{
    struct bsc_peek_info *peek_info = &(node->cb_data->peek_info);

    if (node->bytes_expected) {
        peek_info->response.data = (void *)data;
        if (peek_info->user_cb != NULL)
            peek_info->user_cb(client, peek_info);
    }
    else {
        peek_info->response.code = bsp_get_peek_res(data, &(peek_info->response.id), &(peek_info->response.bytes));
        if (peek_info->response.code == BSP_PEEK_RES_FOUND)
            node->bytes_expected = peek_info->response.bytes;
        else {
            if (peek_info->user_cb != NULL)
                peek_info->user_cb(client, peek_info);
        }
    }
}

static bool insert_tube_before(const char *name, struct bsc_tube_list **l)
{
    struct bsc_tube_list *newl = NULL;

    if ( ( newl = (struct bsc_tube_list *)malloc(sizeof(struct bsc_tube_list)) ) == NULL )
        return false;
    else {
        newl->name  = strdup(name);
        newl->next  = *l;
        *l          = newl;
        return true;
    }
}

static bool insert_tube_after(const char *name, struct bsc_tube_list *l)
{
    struct bsc_tube_list *newl = NULL;

    if ( ( newl = (struct bsc_tube_list *)malloc(sizeof(struct bsc_tube_list)) ) == NULL )
        return false;
    else {
        newl->name  = strdup(name);
        newl->next  = l->next;
        l->next     = newl;
        return true;
    }
}

#ifdef __cplusplus
    }
#endif
