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

#include <stdbool.h>
#include <ev.h>
#include <beanstalkclient.h>
#include "evbuffer.h"
#include "evvector.h"

#define BSC_ENQ_CMD(cmd, bsc, cb, on_error, ...) do {                       \
    char   *c = NULL;                                                       \
    size_t c_len;                                                           \
    if ( ( c = bsc_gen_ ## cmd ## _cmd(&c_len, ## __VA_ARGS__) ) == NULL )  \
        goto on_error;                                                      \
    if ( !evbuffer_put( (bsc)->buf, c, c_len, (cb), 1 ) ) {                 \
        free(c);                                                            \
        goto on_error;                                                      \
    }                                                                       \
} while (0)

struct _evbsc {
    int      fd;
    ev_io    rw;
    ev_io    ww;
    char     *host;
    char     *port;
    evbuffer *buf;
    evvector *vec;
    unsigned reconnect_attempts;
    size_t   vec_min;
};

typedef struct _evbsc evbsc;

evbsc *evbsc_new( struct ev_loop *loop, char *host, char *port, unsigned reconnect_attempts,
                  size_t buf_len, size_t vec_len, size_t vec_rlen, size_t vec_min, char **errorstr );

void evbsc_free(evbsc *bsc);
bool evbsc_connect(evbsc *bsc, struct ev_loop *loop, char **errorstr);
void evbsc_disconnect(evbsc *bsc, struct ev_loop *loop);
bool evbsc_reconnect(evbsc *bsc, struct ev_loop *loop);

#endif /* EVBSC_H */
