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

#include <beanstalkproto.h>
#include <ioqueue.h>
#include "ivector.h"
#include <arrayqueue.h>

struct _bsc;
struct _cbq_node;
struct bsc_put_info;
struct bsc_reserve_info;
struct bsc_delete_info;

typedef void (*bsc_put_user_cb)(struct _bsc *, struct bsc_put_info *cmd_info);

struct bsc_put_info {
    void *user_data;
    bsc_put_user_cb user_cb;
    struct {
        uint32_t    priority;
        uint32_t    delay;
        uint32_t    ttr;
        size_t      bytes;
        const char *data;
        bool        autofree;
    } request;
    struct {
        bsp_response_t code;
        uint64_t       id;
    } response;
};

typedef void (*bsc_reserve_user_cb)(struct _bsc *, struct bsc_reserve_info *cmd_info);

struct bsc_reserve_info {
    void *user_data;
    bsc_reserve_user_cb user_cb;
    struct {
        int32_t    timeout;
    } request;
    struct {
        bsp_response_t code;
        uint64_t       id;
        size_t         bytes;
        void          *data;
    } response;
};

typedef void (*bsc_delete_user_cb)(struct _bsc *, struct bsc_delete_info *cmd_info);

struct bsc_delete_info {
    void *user_data;
    bsc_delete_user_cb user_cb;
    struct {
        uint64_t       id;
    } request;
    struct {
        bsp_response_t code;
    } response;
};

union bsc_cmd_info {
    struct bsc_put_info     put_info;
    struct bsc_reserve_info reserve_info;
    struct bsc_delete_info  delete_info;
    void *user_data;
};

typedef void (*bsc_cb_p_t)(struct _bsc *, struct _cbq_node *, void *, size_t);

struct _cbq_node {
    void   *data;
    int    len;
    bool   is_allocated;
    size_t bytes_expected;
    off_t  outq_offset;
    union  bsc_cmd_info *cb_data;
    bsc_cb_p_t cb;
};

DEFINE_STRUCT_QUEUE(_cbq_queue, struct _cbq_node);

typedef struct _cbq_node queue_node;
typedef struct _cbq_queue queue;

#ifndef BSC_PUT_FINISHED_CODE
#define BSC_PUT_FINISHED_CODE(q) 0
#endif

#define QUEUE_FIN_PUT(q) (AQUEUE_FIN_PUT(q), BSC_PUT_FINISHED_CODE(q))

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

#define BSC_ENQ_CMD(client, cmd, u_cb, u_data, ...) (                                    \
    AQUEUE_NODES_FREE((client)->outq) - (client)->outq_offset < 1 ? BSC_ERROR_QUEUE_FULL \
    : ( ( AQUEUE_FRONT_NV((client)->cbq)->data                                           \
        = bsp_gen_ ## cmd ## _cmd( &(AQUEUE_FRONT_NV((client)->cbq)->len),               \
            &(AQUEUE_FRONT_NV((client)->cbq)->is_allocated), ## __VA_ARGS__) ) == NULL   \
        ? BSC_ERROR_MEMORY                                                               \
        : ( IOQ_PUT_NV( (client)->outq, AQUEUE_FRONT_NV((client)->cbq)->data,            \
                    AQUEUE_FRONT_NV((client)->cbq)->len, false ),                        \
              AQUEUE_FRONT_NV((client)->cbq)->cb                  = u_cb,                \
              AQUEUE_FRONT_NV((client)->cbq)->cb_data->user_data  = u_data,              \
              AQUEUE_FRONT_NV((client)->cbq)->outq_offset    = 0,                        \
              AQUEUE_FRONT_NV((client)->cbq)->bytes_expected = 0,                        \
              QUEUE_FIN_PUT((client)->cbq),                                              \
              BSC_ERROR_NONE ) ) )

enum _bsc_error_e_t { BSC_ERROR_NONE, BSC_ERROR_INTERNAL, BSC_ERROR_SOCKET, BSC_ERROR_MEMORY, BSC_ERROR_QUEUE_FULL };

typedef enum _bsc_error_e_t bsc_error_t;

typedef void (*error_callback_p_t)(struct _bsc *client, bsc_error_t);

struct _bsc {
    int      fd;
    char    *host;
    char    *port;
    queue   *cbq;
    ioq     *outq;
    ivector *vec;
    size_t   vec_min;
    void    *data;
    size_t   outq_offset;
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
* 
* @param client          bsc instance
* @param user_cb         callback on response
* @param user_data       custom data associated with the callback
* @param priority        job priority
* @param delay           job delay start
* @param ttr             job time to run
* @param bytes           job length
* @param data            job data
* @param free_when_done  flag that indicates weather to free data when done with callback
* 
* @return            the error code
*/
bsc_error_t bsc_put(bsc            *client,
                    bsc_put_user_cb user_cb,
                    void           *user_data,
                    uint32_t        priority,
                    uint32_t        delay,
                    uint32_t        ttr,
                    size_t          bytes,
                    const char     *data,
                    bool            free_when_done);

/** 
* reserve a job from the beanstalk server.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param timeout    if < 0: issues normal reserve, else: issues reserve_with_timeout
* 
* @return           the error code
*/
bsc_error_t bsc_reserve(bsc                *client,
                        bsc_reserve_user_cb user_cb,
                        void               *user_data,
                        int32_t             timeout);

/** 
* deletes a job from the beanstalk server.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param id         the id of the job to be deleted
* 
* @return           the error code
*/
bsc_error_t bsc_delete(bsc                *client,
                       bsc_delete_user_cb  user_cb,
                       void               *user_data,
                       uint64_t            id);
#ifdef __cplusplus
    }
#endif
#endif /* BEANSTALKCLIENT_H */
