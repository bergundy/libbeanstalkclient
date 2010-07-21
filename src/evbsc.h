/**
 * =====================================================================================
 * @file   bsc.h
 * @brief  header file for beanstalkclient - a nonblocking implementation
 * @date   07/05/2010 06:50:22 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * @todo   add list/hash of watched tubes for reconnect
 * @todo   change BSC_ENQ_CMD to normal function and make the callback api easier
 * =====================================================================================
 */
#ifndef BEANSTALKCLIENT_H
#define BEANSTALKCLIENT_H

#ifdef __cplusplus
    extern "C" {
#endif

#include <stdbool.h>
#include <beanstalkproto.h>
#include <ioqueue.h>
#include "evvector.h"

struct _bsc;
struct _arrayqueue_node;

typedef void (*callback_p_t)(struct _bsc *, struct _arrayqueue_node *, void *, size_t);

struct _arrayqueue_node {
    void   *data;
    size_t len;
    size_t bytes_expected;
    void   *cb_data;
    callback_p_t cb;
};

typedef struct _arrayqueue_node queue_node;

#include "arrayqueue.h"

typedef struct _arrayqueue queue;

#define BSC_ENQ_CMD(cmd, client, callback, callback_data, on_error, ...) do {  \
    char   *c = NULL;                                                          \
    int    c_len;                                                              \
    if ( ( c = bsp_gen_ ## cmd ## _cmd(&c_len, ## __VA_ARGS__) ) == NULL )     \
        goto on_error;                                                         \
    if ( !IOQ_PUT( (client)->outq, c, c_len, 0 ) )       {                     \
        free(c);                                                               \
        goto on_error;                                                         \
    }                                                                          \
    AQUEUE_FRONT_NV((client)->cbq)->data    = (c);                             \
    AQUEUE_FRONT_NV((client)->cbq)->len     = (c_len);                         \
    AQUEUE_FRONT_NV((client)->cbq)->cb      = (callback);                      \
    AQUEUE_FRONT_NV((client)->cbq)->cb_data = (callback_data);                 \
    AQUEUE_FRONT_NV((client)->cbq)->bytes_expected = 0;                        \
    AQUEUE_FIN_PUT((client)->cbq);                                             \
} while (false)

#define QUEUE_FIN_CMD(q) do {      \
    free(AQUEUE_REAR_NV(q)->data); \
    AQUEUE_FIN_GET(q);             \
} while (false)

#define BSC_DEFAULT_BUFFER_SIZE 1024
#define BSC_DEFAULT_VECTOR_SIZE 1024
#define BSC_DEFAULT_VECTOR_MIN  256

#define bsc_new_w_defaults(host, port, onerror, errorstr)   \
    ( bsc_new( (host), (port), (onerror),                   \
        BSC_DEFAULT_BUFFER_SIZE,                            \
        BSC_DEFAULT_VECTOR_SIZE,                            \
        BSC_DEFAULT_VECTOR_MIN,                             \
        (errorstr) ) )

enum _bsc_error_e_t { BSC_ERROR_INTERNAL, BSC_ERROR_SOCKET };

typedef enum _bsc_error_e_t bsc_error_t;

typedef void (*error_callback_p_t)(struct _bsc *client, bsc_error_t);

struct _bsc {
    int      fd;
    char     *host;
    char     *port;
    queue    *cbq;
    ioq      *outq;
    evvector *vec;
    size_t   vec_min;
    void     *data;
    error_callback_p_t onerror;
};

typedef struct _bsc bsc;

bsc *bsc_new( const char *host, const char *port, error_callback_p_t onerror,
                  size_t buf_len, size_t vec_len, size_t vec_min, char **errorstr );

void bsc_free(bsc *client);
bool bsc_connect(bsc *client, char **errorstr);
void bsc_disconnect(bsc *client);
bool bsc_reconnect(bsc *client, char **errorstr);
void bsc_write(bsc *client);
void bsc_read(bsc *client);

#ifdef __cplusplus
    }
#endif
#endif /* BEANSTALKCLIENT_H */
