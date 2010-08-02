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
struct bsc_use_info;
struct bsc_reserve_info;
struct bsc_delete_info;
struct bsc_release_info;
struct bsc_bury_info;
struct bsc_touch_info;
struct bsc_watch_info;
struct bsc_ignore_info;
struct bsc_peek_info;

typedef void (*bsc_put_user_cb)(struct _bsc *, struct bsc_put_info *);

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

typedef void (*bsc_use_user_cb)(struct _bsc *, struct bsc_use_info *);

struct bsc_use_info {
    void *user_data;
    bsc_use_user_cb user_cb;
    struct {
        const char *tube;
    } request;
    struct {
        bsp_response_t code;
        char          *tube;
    } response;
};

typedef void (*bsc_reserve_user_cb)(struct _bsc *, struct bsc_reserve_info *);

struct bsc_reserve_info {
    void *user_data;
    bsc_reserve_user_cb user_cb;
    struct {
        int32_t timeout;
    } request;
    struct {
        bsp_response_t code;
        uint64_t       id;
        size_t         bytes;
        void          *data;
    } response;
};

typedef void (*bsc_delete_user_cb)(struct _bsc *, struct bsc_delete_info *);

struct bsc_delete_info {
    void *user_data;
    bsc_delete_user_cb user_cb;
    struct {
        uint64_t id;
    } request;
    struct {
        bsp_response_t code;
    } response;
};

typedef void (*bsc_release_user_cb)(struct _bsc *, struct bsc_release_info *);

struct bsc_release_info {
    void *user_data;
    bsc_release_user_cb user_cb;
    struct {
        uint64_t id;
        uint32_t priority;
        uint32_t delay;
    } request;
    struct {
        bsp_response_t code;
    } response;
};

typedef void (*bsc_bury_user_cb)(struct _bsc *, struct bsc_bury_info *);

struct bsc_bury_info {
    void *user_data;
    bsc_bury_user_cb user_cb;
    struct {
        uint64_t id;
        uint32_t priority;
    } request;
    struct {
        bsp_response_t code;
    } response;
};

typedef void (*bsc_touch_user_cb)(struct _bsc *, struct bsc_touch_info *);

struct bsc_touch_info {
    void *user_data;
    bsc_touch_user_cb user_cb;
    struct {
        uint64_t id;
    } request;
    struct {
        bsp_response_t code;
    } response;
};

typedef void (*bsc_watch_user_cb)(struct _bsc *, struct bsc_watch_info *);

struct bsc_watch_info {
    void *user_data;
    bsc_watch_user_cb user_cb;
    struct {
        const char *tube;
    } request;
    struct {
        bsp_response_t code;
        uint32_t count;
    } response;
};

typedef void (*bsc_ignore_user_cb)(struct _bsc *, struct bsc_ignore_info *);

struct bsc_ignore_info {
    void *user_data;
    bsc_ignore_user_cb user_cb;
    struct {
        const char *tube;
    } request;
    struct {
        bsp_response_t code;
        uint32_t count;
    } response;
};

enum peek_cmd_e {
    ID, READY, DELAYED, BURIED
} peek_type;

typedef void (*bsc_peek_user_cb)(struct _bsc *, struct bsc_peek_info *);

struct bsc_peek_info {
    void *user_data;
    bsc_peek_user_cb user_cb;
    struct {
        enum peek_cmd_e peek_type;
        uint64_t id;
    } request;
    struct {
        bsp_response_t code;
        uint64_t       id;
        size_t         bytes;
        void          *data;
    } response;
};

union bsc_cmd_info {
    struct bsc_put_info     put_info;
    struct bsc_use_info     use_info;
    struct bsc_reserve_info reserve_info;
    struct bsc_delete_info  delete_info;
    struct bsc_release_info release_info;
    struct bsc_bury_info    bury_info;
    struct bsc_touch_info   touch_info;
    struct bsc_watch_info   watch_info;
    struct bsc_ignore_info  ignore_info;
    struct bsc_peek_info    peek_info;
};

typedef void (*bsc_cb_p_t)(struct _bsc *, struct _cbq_node *, const char *, size_t);

struct _cbq_node {
    void  *data;
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

#define BSC_BUFFER_NODES_FREE(client)   \
  ( AQUEUE_NODES_FREE((client)->outq)   \
  - (client)->outq_offset )

#define QUEUE_FIN_PUT(client) (AQUEUE_FIN_PUT((client)->cbq), (client)->buffer_fill_cb != NULL ? (client)->buffer_fill_cb(client) : 0 )

#define QUEUE_FIN_CMD(q) do {            \
    if (AQUEUE_REAR_NV(q)->is_allocated) \
        free(AQUEUE_REAR_NV(q)->data);   \
    AQUEUE_FIN_GET(q);                   \
} while (false)

#define BSC_DEFAULT_BUFFER_SIZE 1024
#define BSC_DEFAULT_VECTOR_SIZE 1024
#define BSC_DEFAULT_VECTOR_MIN  256
#define BSC_DEFAULT_TUBE        "default"
#define BSC_RESERVE_NO_TIMEOUT  -1

#define bsc_new_w_defaults(host, port, tube, onerror, errorstr)  \
    ( bsc_new( (host), (port), (tube), (onerror),                \
        BSC_DEFAULT_BUFFER_SIZE,                                 \
        BSC_DEFAULT_VECTOR_SIZE,                                 \
        BSC_DEFAULT_VECTOR_MIN,                                  \
        (errorstr) ) )

enum _bsc_error_e_t { BSC_ERROR_NONE, BSC_ERROR_INTERNAL, BSC_ERROR_SOCKET, BSC_ERROR_MEMORY, BSC_ERROR_QUEUE_FULL };

typedef enum _bsc_error_e_t bsc_error_t;

typedef void (*error_callback_p_t)(struct _bsc *, bsc_error_t);
typedef int (*bsc_buffer_fill_cb)(struct _bsc *);

struct bsc_tube_list {
    char   *name;
    struct bsc_tube_list *next;
};

struct _bsc {
    int      fd;
    char    *host;
    char    *port;
    char    *default_tube;
    queue   *cbq;
    queue   *tubecbq;
    ioq     *outq;
    ioq     *tubeq;
    ivector *vec;
    size_t   vec_min;
    void    *data;
    size_t   outq_offset;
    struct bsc_tube_list *watched_tubes;
    unsigned watched_tubes_count;
    bsc_buffer_fill_cb buffer_fill_cb;
    error_callback_p_t onerror;
};

typedef struct _bsc bsc;

bsc *bsc_new(const char *host, const char *port, const char *default_tube,
             error_callback_p_t onerror, size_t buf_len,
             size_t vec_len, size_t vec_min, char **errorstr);

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
* instructs the beanstalk server to use "tube" for putting jobs.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param tube       the name of tube to use
* 
* @return           the error code
*/
bsc_error_t bsc_use(bsc                *client,
                    bsc_use_user_cb     user_cb,
                    void               *user_data,
                    const char         *tube);

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

/** 
* releases a job from the beanstalk server.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param id         the id of the job to be released
* 
* @return           the error code
*/
bsc_error_t bsc_release(bsc                 *client,
                        bsc_release_user_cb  user_cb,
                        void                *user_data,
                        uint64_t             id, 
                        uint32_t             priority,
                        uint32_t             delay);

/** 
* buries a job.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param id         the id of the job to be buried
* @param priority   the new priority to assign to the job
* 
* @return           the error code
*/
bsc_error_t bsc_bury(bsc                *client,
                     bsc_bury_user_cb    user_cb,
                     void               *user_data,
                     uint64_t            id,
                     uint32_t            priority);

/** 
* touches a job.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param id         the id of the job to touch
* 
* @return           the error code
*/
bsc_error_t bsc_touch(bsc                *client,
                      bsc_touch_user_cb   user_cb,
                      void               *user_data,
                      uint64_t            id);

/** 
* adds the named tube to the watch list for the current connection.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param tube       a name of a tube to add to the watch list
* 
* @return           the error code
*/
bsc_error_t bsc_watch(bsc                *client,
                      bsc_watch_user_cb   user_cb,
                      void               *user_data,
                      const char         *tube);

/** 
* removes the named tube from the watch list for the current connection.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param tube       a name of a tube to remove from the watch list
* 
* @return           the error code
*/
bsc_error_t bsc_ignore(bsc                *client,
                       bsc_ignore_user_cb  user_cb,
                       void               *user_data,
                       const char         *tube);

/** 
* inspect a job in the system.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param peek_type  the type of peek command to issue
* @param id         if peek type is ID refers the issued command will refer to this id
* 
* @return           the error code
*/
bsc_error_t bsc_peek(bsc                *client,
                     bsc_peek_user_cb    user_cb,
                     void               *user_data,
                     enum peek_cmd_e     peek_type,
                     uint64_t            id);

#ifdef __cplusplus
    }
#endif
#endif /* BEANSTALKCLIENT_H */
