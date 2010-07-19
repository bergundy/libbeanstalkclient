/**
 * =====================================================================================
 * @file   evbsc.h
 * @brief  header file for evbsc - libev implementation of the beanstalk client library
 * @date   07/05/2010 06:50:22 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */
#ifndef EVBSC_H
#define EVBSC_H 

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <ev.h>
#include <beanstalkclient.h>
#include <ioqueue.h>
#include "evvector.h"

struct _evbsc;

struct _arrayqueue_node {
    char   *data;
    size_t len;
    size_t bytes_expected;
    void (*cb)( struct _evbsc *, struct _arrayqueue_node *, char *, size_t);
};

typedef struct _arrayqueue_node queue_node;
#include "arrayqueue.h"

typedef void (*callback_p_t)(struct _evbsc *bsc, queue_node *node, char *, size_t);

typedef struct _arrayqueue queue;

#define BSC_ENQ_CMD(cmd, bsc, callback, on_error, ...) do {                 \
    char   *c = NULL;                                                       \
    int    c_len;                                                           \
    if ( ( c = bsc_gen_ ## cmd ## _cmd(&c_len, ## __VA_ARGS__) ) == NULL )  \
        goto on_error;                                                      \
    if ( !IOQ_PUT( (bsc)->outq, c, c_len, 0 ) )       {                     \
        free(c);                                                            \
        goto on_error;                                                      \
    }                                                                       \
    AQUEUE_FRONT_NV((bsc)->cbq)->data = (c);                                \
    AQUEUE_FRONT_NV((bsc)->cbq)->len  = (c_len);                            \
    AQUEUE_FRONT_NV((bsc)->cbq)->cb   = (callback);                         \
    AQUEUE_FRONT_NV((bsc)->cbq)->bytes_expected = 0;                        \
    AQUEUE_FIN_PUT((bsc)->cbq);                                             \
    ev_io_start((bsc)->loop, &((bsc)->ww));                                 \
} while (false)

#define QUEUE_FIN_CMD(q) do {      \
    free(AQUEUE_REAR_NV(q)->data); \
    AQUEUE_FIN_GET(q);             \
} while (false)

#define EVBSC_DEFAULT_BUFFER_SIZE 1024
#define EVBSC_DEFAULT_VECTOR_SIZE 1024
#define EVBSC_DEFAULT_VECTOR_MIN  256

#define evbsc_new_w_defaults(loop, host, port, onerror, errorstr)   \
    ( evbsc_new( (loop), (host), (port), (onerror),                 \
        EVBSC_DEFAULT_BUFFER_SIZE,                                  \
        EVBSC_DEFAULT_VECTOR_SIZE,                                  \
        EVBSC_DEFAULT_VECTOR_MIN,                                   \
        (errorstr) ) )

enum _evbsc_error_e_t { EVBSC_ERROR_INTERNAL, EVBSC_ERROR_SOCKET };
typedef enum _evbsc_error_e_t evbsc_error_t;

typedef void (*error_callback_p_t)(struct _evbsc *bsc, evbsc_error_t);

struct _evbsc {
    int      fd;
    ev_io    rw;
    ev_io    ww;
    char     *host;
    char     *port;
    queue    *cbq;
    ioq      *outq;
    evvector *vec;
    size_t   vec_min;
    error_callback_p_t onerror;
    struct ev_loop     *loop;
};

typedef struct _evbsc evbsc;

evbsc *evbsc_new( struct ev_loop *loop, const char *host, const char *port, error_callback_p_t onerror,
                  size_t buf_len, size_t vec_len, size_t vec_min, char **errorstr );

void evbsc_free(evbsc *bsc);
bool evbsc_connect(evbsc *bsc, char **errorstr);
void evbsc_disconnect(evbsc *bsc);
bool evbsc_reconnect(evbsc *bsc, char **errorstr);

#ifdef __cplusplus
    }
#endif
#endif /* EVBSC_H */
