/**
 * =====================================================================================
 * @file   beanstalkclient.h
 * @brief  header file for beanstalkclient - a nonblocking implementation
 * @date   07/05/2010 06:50:22 PM
 * @author   Roey Berman, (roey.berman@gmail.com)
 * @version  1.0
 *
 * Copyright (c) 2010, Roey Berman, (roeyb.berman@gmail.com)
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by Roey Berman.
 * 4. Neither the name of Roey Berman nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY ROEY BERMAN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ROEY BERMAN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
struct bsc_kick_info;
struct bsc_pause_tube_info;
struct bsc_stats_job_info;
struct bsc_stats_tube_info;
struct bsc_server_stats_info;
struct bsc_list_tubes_info;

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
    BSC_PEEK_T_ID, BSC_PEEK_T_READY, BSC_PEEK_T_DELAYED, BSC_PEEK_T_BURIED
};

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

typedef void (*bsc_kick_user_cb)(struct _bsc *, struct bsc_kick_info *);

struct bsc_kick_info {
    void *user_data;
    bsc_kick_user_cb user_cb;
    struct {
        uint32_t bound;
    } request;
    struct {
        bsp_response_t code;
        uint32_t       count;
    } response;
};

typedef void (*bsc_pause_tube_user_cb)(struct _bsc *, struct bsc_pause_tube_info *);

struct bsc_pause_tube_info {
    void *user_data;
    bsc_pause_tube_user_cb user_cb;
    struct {
        const char *tube;
        uint32_t    delay;
    } request;
    struct {
        bsp_response_t code;
    } response;
};

typedef void (*bsc_stats_job_user_cb)(struct _bsc *, struct bsc_stats_job_info *);

struct bsc_stats_job_info {
    void *user_data;
    bsc_stats_job_user_cb user_cb;
    struct {
        uint64_t id;
    } request;
    struct {
        bsp_response_t code;
        size_t         bytes;
        char          *data;
        bsp_job_stats *stats;
    } response;
};

typedef void (*bsc_stats_tube_user_cb)(struct _bsc *, struct bsc_stats_tube_info *);

struct bsc_stats_tube_info {
    void *user_data;
    bsc_stats_tube_user_cb user_cb;
    struct {
        const char *tube;
    } request;
    struct {
        bsp_response_t  code;
        size_t          bytes;
        char           *data;
        bsp_tube_stats *stats;
    } response;
};

typedef void (*bsc_server_stats_user_cb)(struct _bsc *, struct bsc_server_stats_info *);

struct bsc_server_stats_info {
    void *user_data;
    bsc_server_stats_user_cb user_cb;
    struct {
        bsp_response_t    code;
        size_t            bytes;
        char             *data;
        bsp_server_stats *stats;
    } response;
};

typedef void (*bsc_list_tubes_user_cb)(struct _bsc *, struct bsc_list_tubes_info *);

struct bsc_list_tubes_info {
    void *user_data;
    bsc_list_tubes_user_cb user_cb;
    struct {
        bsp_response_t  code;
        size_t          bytes;
        char           *data;
        char          **tubes;
    } response;
};

union bsc_cmd_info {
    struct bsc_put_info             put_info;
    struct bsc_use_info             use_info;
    struct bsc_reserve_info         reserve_info;
    struct bsc_delete_info          delete_info;
    struct bsc_release_info         release_info;
    struct bsc_bury_info            bury_info;
    struct bsc_touch_info           touch_info;
    struct bsc_watch_info           watch_info;
    struct bsc_ignore_info          ignore_info;
    struct bsc_peek_info            peek_info;
    struct bsc_kick_info            kick_info;
    struct bsc_pause_tube_info      pause_tube_info;
    struct bsc_stats_job_info       stats_job_info;
    struct bsc_stats_tube_info      stats_tube_info;
    struct bsc_server_stats_info    server_stats_info;
    struct bsc_list_tubes_info      list_tubes_info;
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

DEFINE_STRUCT_QUEUE(_cbq, struct _cbq_node);

typedef struct _cbq_node cbq_node;
typedef struct _cbq      cbq;

#define BSC_BUFFER_NODES_FREE(client)   \
  ( AQUEUE_NODES_FREE((client)->outq)   \
  - (client)->outq_offset )

#define QUEUE_FIN_PUT(client) (AQUEUE_FIN_PUT((client)->cbqueue), (client)->buffer_fill_cb != NULL ? (client)->buffer_fill_cb(client) : 0 )

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
#define BSC_ERRSTR_LEN          512

#define bsc_new_w_defaults(host, port, tube, onerror, errorstr)  \
    ( bsc_new( (host), (port), (tube), (onerror),                \
        BSC_DEFAULT_BUFFER_SIZE,                                 \
        BSC_DEFAULT_VECTOR_SIZE,                                 \
        BSC_DEFAULT_VECTOR_MIN,                                  \
        (errorstr) ) )

enum _bsc_error_e_t { BSC_ERROR_NONE, BSC_ERROR_INTERNAL, BSC_ERROR_SOCKET, BSC_ERROR_MEMORY, BSC_ERROR_QUEUE_FULL };

typedef enum _bsc_error_e_t bsc_error_t;

typedef void (*error_callback_p_t)(struct _bsc *, bsc_error_t);
typedef int  (*bsc_buffer_fill_cb)(struct _bsc *);
typedef void (*bsc_conn_cb)(struct _bsc *);

typedef enum { BSC_STATE_DISCONNECTED, BSC_STATE_CONNECTED } bsc_state_t;

struct bsc_tube_list {
    char   *name;
    struct bsc_tube_list *next;
};

struct _bsc {
    int      fd;
    char    *host;
    char    *port;
    char    *default_tube;
    cbq     *cbqueue;
    cbq     *tubecbq;
    ioq     *outq;
    ioq     *tubeq;
    ivector *vec;
    size_t   vec_min;
    void    *data;
    size_t   outq_offset;
    struct bsc_tube_list *watched_tubes;
    bsc_state_t state;
    unsigned watched_tubes_count;
    bsc_buffer_fill_cb buffer_fill_cb;
    bsc_conn_cb pre_disconnect_cb;
    bsc_conn_cb post_connect_cb;
    error_callback_p_t onerror;
};

typedef struct _bsc bsc;

/** 
* creates a new bsc instance and connects it to a beanstalkd.
* 
* @param host           the address of the beanstalkd host
* @param port           the beanstalkd port
* @param default_tube   the tube to use and watch
* @param onerror        callback on error
* @param buf_len        the write queue size (messages not bytes) it does not grow
* @param vec_len        the input buffer initial size (doubles automatically)
* @param vec_min        the input buffer minimum size - if reached size will double
* @param errorstr       a string to store an error in (must be at least BSC_ERRSTR_LEN)
* 
* @return a pointer to the newly allocated bsc
*/
bsc *bsc_new(const char *host, const char *port, const char *default_tube,
             error_callback_p_t onerror, size_t buf_len,
             size_t vec_len, size_t vec_min, char *errorstr);

/** 
* frees all resources taken by the client
*
* @param client  a bsc instance ..
*/
void bsc_free(bsc *client);

/** 
* connects the client to a beanstalk server and restores it's watched/used tubes and incomplete commands.
* 
* @param client   a bsc instance
* @param errorstr a string to store an error in (must be at least BSC_ERRSTR_LEN)
* 
* @return         the success of the connection
*/
bool bsc_connect(bsc *client, char *errorstr);

/** 
* disconnect the client from the beanstalkd.
* 
* @param client
*/
void bsc_disconnect(bsc *client);

/** 
* calls disconnect and connect.
* 
* @param client    a bsc instance
* @param errorstr  a string to store an error in (must be at least BSC_ERRSTR_LEN)
* 
* @return          the success of connect
*/
bool bsc_reconnect(bsc *client, char *errorstr);

/** 
* call this funcion when the client's fd is ready for writing.
* 
* @param client   a bsc instance
*/
void bsc_write(bsc *client);

/** 
* call this funcion when the client's fd is ready for reading.
* 
* @param client   a bsc instance
*/
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
* @param tube       the name of tube to use, 
*                   if tube needs to be freed - free it in user_cb (struct use_info)
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
* @param priority   a new priority to asign to the job
* @param delay      job delay start
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
* @param tube       a name of a tube to add to the watch list, 
*                   if tube needs to be freed - free it in user_cb (struct watch_info)
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
* @param tube       a name of a tube to remove from the watch list, 
*                   if tube needs to be freed - free it in user_cb (struct ignore_info)
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
* @param id         if peek type is BSC_PEEK_T_ID refers the issued command will refer to this id
* 
* @return           the error code
*/
bsc_error_t bsc_peek(bsc                *client,
                     bsc_peek_user_cb    user_cb,
                     void               *user_data,
                     enum peek_cmd_e     peek_type,
                     uint64_t            id);

/** 
* moves jobs into the ready queue. 
* If there are any buried jobs, it will only kick buried jobs.
* Otherwise it will kick delayed jobs.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param bound      an integer upper bound on the number of jobs to kick
* 
* @return           the error code
*/
bsc_error_t bsc_kick(bsc                *client,
                     bsc_kick_user_cb    user_cb,
                     void               *user_data,
                     uint32_t            bound);

/** 
* The pause-tube command can delay any new job being reserved for a given time.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param tube       the tube to pause
* @param delay      is an integer number of seconds to wait before reserving any more jobs from the queue
* 
* @return           the error code
*/
bsc_error_t bsc_pause_tube(bsc                    *client, 
                           bsc_pause_tube_user_cb  user_cb,
                           void                   *user_data,
                           const char             *tube,
                           uint32_t                delay);

/** 
* gives statistical information about the specified job.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param id         the id of the job to get stats for
* 
* @return           the error code
*/
bsc_error_t bsc_stats_job(bsc                     *client,
                          bsc_stats_job_user_cb    user_cb,
                          void                    *user_data,
                          uint64_t                 id);

/** 
* gives statistical information about the specified tube.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* @param tube       the name of the tube to get stats for, 
*                   if tube needs to be freed - free it in user_cb (struct stats_tube_info)
* 
* @return           the error code
*/
bsc_error_t bsc_stats_tube(bsc                     *client,
                           bsc_stats_tube_user_cb   user_cb,
                           void                    *user_data,
                           const char              *tube);

/** 
* gives statistical information about the server client is connected to.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* 
* @return           the error code
*/
bsc_error_t bsc_server_stats(bsc                       *client,
                             bsc_server_stats_user_cb   user_cb,
                             void                      *user_data);

/** 
* The list-tubes command returns a list of all existing tubes.
* 
* @param client     bsc instance
* @param user_cb    callback on response
* @param user_data  custom data associated with the callback
* 
* @return           the error code
*/
bsc_error_t bsc_list_tubes(bsc                    *client,
                           bsc_list_tubes_user_cb  user_cb,
                           void                   *user_data);

#ifdef __cplusplus
    }
#endif
#endif /* BEANSTALKCLIENT_H */
