/**
 * =====================================================================================
 * @file   beanstalkclient.h
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
#include "ivector.h"

struct _bsc;
struct _arrayqueue_node;

typedef void (*bsc_cb_p_t)(struct _bsc *, struct _arrayqueue_node *, void *, size_t);

struct _arrayqueue_node {
    void   *data;
    int    len;
    bool   is_allocated;
    size_t bytes_expected;
    void  *cb_data;
    bsc_cb_p_t cb;
};

typedef struct _arrayqueue_node queue_node;

#include "arrayqueue.h"

typedef struct _arrayqueue queue;

#define QUEUE_FIN_CMD(q) do {            \
    if (AQUEUE_REAR_NV(q)->is_allocated) \
        free(AQUEUE_REAR_NV(q)->data);   \
    AQUEUE_FIN_GET(q);                   \
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

#define BSC_ENQ_CMD(client, cmd, user_cb, user_data, ...) (                             \
    IOQ_FULL((client)->outq) ? BSC_ERROR_QUEUE_FULL :                                   \
    ( ( AQUEUE_FRONT_NV((client)->cbq)->data                                            \
        = bsp_gen_ ## cmd ## _cmd( &(AQUEUE_FRONT_NV((client)->cbq)->len),              \
            &(AQUEUE_FRONT_NV((client)->cbq)->is_allocated), ## __VA_ARGS__) ) == NULL  \
        ? BSC_ERROR_MEMORY                                                              \
        : ( IOQ_PUT_NV( (client)->outq, AQUEUE_FRONT_NV((client)->cbq)->data,           \
                    AQUEUE_FRONT_NV((client)->cbq)->len, false ),                       \
              AQUEUE_FRONT_NV((client)->cbq)->cb                  = user_cb,            \
              AQUEUE_FRONT_NV((client)->cbq)->cb_data             = user_data,          \
              AQUEUE_FRONT_NV((client)->cbq)->bytes_expected = 0,                       \
              AQUEUE_FIN_PUT((client)->cbq),                                            \
              BSC_ERROR_NONE ) ) )

enum _bsc_error_e_t { BSC_ERROR_NONE, BSC_ERROR_INTERNAL, BSC_ERROR_SOCKET, BSC_ERROR_MEMORY, BSC_ERROR_QUEUE_FULL };

typedef enum _bsc_error_e_t bsc_error_t;

typedef void (*error_callback_p_t)(struct _bsc *client, bsc_error_t);

struct _bsc {
    int      fd;
    char     *host;
    char     *port;
    queue    *cbq;
    ioq      *outq;
    ivector *vec;
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

/** 
* puts a job into the beanstalk server.
* data will NOT be freed after write!
* 
* @param client          bsc instance
* @param user_cb         callback on response
* @param user_data       custom data associated with callbacks
* @param priority        job priority
* @param delay           job delay start
* @param ttr             job time to run
* @param bytes           job length
* @param data            job data
* 
* @return            the error
*/
bsc_error_t bsc_put(bsc        *client,
                    bsc_cb_p_t  user_cb,
                    void       *user_data,
                    uint32_t    priority,
                    uint32_t    delay,
                    uint32_t    ttr,
                    size_t      bytes,
                    const char *data);

#ifdef __cplusplus
    }
#endif
#endif /* BEANSTALKCLIENT_H */
