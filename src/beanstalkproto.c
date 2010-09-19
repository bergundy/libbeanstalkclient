/**
 * =====================================================================================
 * @file     beanstalkproto.c
 * @brief    beanstalk protocol related functions
 * @date     08/09/2010
 * @author   Roey Berman, (roey.berman@gmail.com)
 * @version  1.1
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

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "beanstalkproto.h"

#define  CSTRLEN(cstr) (sizeof(cstr)/sizeof(char)-1)
#define  MACRO2STR_(m) #m
#define  MACRO2STR(m) MACRO2STR_(m)
#define  UINT32_STRL ( CSTRLEN(MACRO2STR(UINT32_MAX)) )
#define  UINT64_STRL ( CSTRLEN(MACRO2STR(UINT64_MAX)) )


static const char *bsp_response_str[] = {
    /* general errors */
    "OUT_OF_MEMORY", "INTERNAL_ERROR", "BAD_FORMAT", "UNKNOWN_COMMAND",
    /* general responses */
    "OK", "BURIED", "NOT_FOUND", "WATCHING",
    /* put cmd results */
    "INSERTED", "EXPECTED_CRLF", "JOB_TOO_BIG", "DRAINING", 
    /* use cmd result */
    "USING",
    /* reserve cmd result */
    "RESERVED", "DEADLINE_SOON", "TIMED_OUT",
    /* delete cmd result */
    "DELETED",
    /* release cmd result */
    "RELEASED",
    /* touch cmd result */
    "TOUCHED",
    /* ignore cmd result */
    "NOT_IGNORED",
    /* peek cmd result */
    "FOUND",
    /* kick cmd result */
    "KICKED",
    /* pause-tube cmd result */
    "PAUSED"
};

static const size_t bsp_response_strlen[] = {
    CSTRLEN("OUT_OF_MEMORY"), CSTRLEN("INTERNAL_ERROR"), CSTRLEN("BAD_FORMAT"), CSTRLEN("UNKNOWN_COMMAND"),
    /* general responses */
    CSTRLEN("OK"), CSTRLEN("BURIED"), CSTRLEN("NOT_FOUND"), CSTRLEN("WATCHING"),
    /* put cmd results */
    CSTRLEN("INSERTED"), CSTRLEN("EXPECTED_CRLF"), CSTRLEN("JOB_TOO_BIG"), CSTRLEN("DRAINING"), 
    /* use cmd result */
    CSTRLEN("USING"),
    /* reserve cmd result */
    CSTRLEN("RESERVED"), CSTRLEN("DEADLINE_SOON"), CSTRLEN("TIMED_OUT"),
    /* delete cmd result */
    CSTRLEN("DELETED"),
    /* release cmd result */
    CSTRLEN("RELEASED"),
    /* touch cmd result */
    CSTRLEN("TOUCHED"),
    /* ignore cmd result */
    CSTRLEN("NOT_IGNORED"),
    /* peek cmd result */
    CSTRLEN("FOUND"),
    /* kick cmd result */
    CSTRLEN("KICKED"),
    /* pause-tube cmd result */
    CSTRLEN("PAUSED")
};

#define GEN_STATIC_CMD(cmd_name, str)                                \
char *bsp_gen_ ## cmd_name ## _cmd(int *cmd_len, bool *is_allocated) \
{                                                                    \
    static const char cmd[] = (str);                                 \
    *cmd_len = CSTRLEN(cmd);                                         \
    *is_allocated = false;                                           \
    return (char *)cmd;                                              \
}

#define INIT_CMD_MALLOC(...)                                            \
    char *cmd   = NULL;                                                 \
    *is_allocated = true;                                               \
    if ( ( cmd = (char *)malloc( sizeof(char) * alloc_len ) ) == NULL ) \
        return NULL;                                                    \
    if ( ( *cmd_len = sprintf(cmd, format, ##__VA_ARGS__ ) ) < 0 ) {    \
        free(cmd);                                                      \
        return NULL;                                                    \
    }                                                                   \
    return cmd;


#define GET_ID_BYTES                                                                \
    p =  (char *)response + bsp_response_strlen[response_t] + 1;                    \
    char *p_tmp = NULL;                                                             \
    *id = strtoull(p, &p_tmp, 10);                                                  \
    if ( ( p = p_tmp ) == NULL)                                                     \
        response_t = BSC_RES_UNRECOGNIZED;                                          \
    else {                                                                          \
        p_tmp = NULL;                                                               \
        *bytes = strtoul(p, &p_tmp, 10);                                            \
        if ( ( p = p_tmp ) == NULL)                                                 \
            response_t = BSC_RES_UNRECOGNIZED;                                      \
    } 

static const bsc_response_t bsp_general_error_responses[] = {
     BSC_RES_OUT_OF_MEMORY,
     BSC_RES_INTERNAL_ERROR,
     BSC_RES_BAD_FORMAT,
     BSC_RES_UNKNOWN_COMMAND
};

#define bsp_get_response_t( response, possibilities )                                \
    register unsigned int i;                                                         \
    response_t = BSC_RES_UNRECOGNIZED;                                               \
    for (i = 0; i < sizeof(possibilities)/sizeof(bsc_response_t); ++i)               \
        if ( ( strncmp(response, bsp_response_str[possibilities[i]],                 \
                bsp_response_strlen[possibilities[i]]) ) == 0 )                      \
        {                                                                            \
            response_t = possibilities[i];                                           \
            goto done;                                                               \
        }                                                                            \
    for (i = 0; i < sizeof(bsp_general_error_responses)/sizeof(bsc_response_t); ++i) \
        if ( ( strncmp(response, bsp_response_str[bsp_general_error_responses[i]],   \
                bsp_response_strlen[bsp_general_error_responses[i]]) ) == 0 )        \
        {                                                                            \
            response_t = bsp_general_error_responses[i];                             \
            goto done;                                                               \
        }                                                                            \
    done:

/*-----------------------------------------------------------------------------
 * producer methods
 *-----------------------------------------------------------------------------*/

char *bsp_gen_put_hdr(int       *hdr_len,
                      bool *is_allocated,
                      uint32_t   priority,
                      uint32_t   delay,
                      uint32_t   ttr,
                      size_t     bytes)
{
    static const char   format[]  = "put %u %u %u %u\r\n";
    static const size_t alloc_len = CSTRLEN(format) + (UINT32_STRL - CSTRLEN("%u")) * 4 + 1;

    *is_allocated = true;
    char *hdr   = NULL;

    if ( ( hdr = (char *)malloc( sizeof(char) * alloc_len ) ) == NULL )
        return NULL;

    if ( ( *hdr_len = sprintf(hdr, format, priority, delay, ttr, bytes) ) < 0 ) {
        free(hdr);
        return NULL;
    }
    return hdr;
}
    
bsc_response_t bsp_get_put_res(const char *response, uint64_t *id)
{
    static const bsc_response_t bsp_put_cmd_responses[] = {
         BSC_PUT_RES_INSERTED,
         BSC_RES_BURIED,
         BSC_PUT_RES_EXPECTED_CRLF,
         BSC_PUT_RES_JOB_TOO_BIG, 
         BSC_PUT_RES_DRAINING 
    };

    bsc_response_t response_t;

    bsp_get_response_t(response, bsp_put_cmd_responses);

    if ( response_t == BSC_PUT_RES_INSERTED || response_t == BSC_RES_BURIED )
        *id = strtoull(response+bsp_response_strlen[response_t], NULL, 10);

    return response_t;
}

char *bsp_gen_use_cmd(int *cmd_len, bool *is_allocated, const char *tube_name)
{
    static const char   format[]   = "use %s\r\n";
    static const size_t format_len = CSTRLEN(format) - CSTRLEN("%s") + 1;

    size_t alloc_len = format_len + strlen(tube_name);
    INIT_CMD_MALLOC(tube_name)
}

bsc_response_t bsp_get_use_res(const char *response, char **tube_name)
{
    static const bsc_response_t bsp_use_cmd_responses[] = {
         BSC_USE_RES_USING
    };

    bsc_response_t response_t;
    char *p1, *p2;

    bsp_get_response_t(response, bsp_use_cmd_responses);

    if ( response_t == BSC_USE_RES_USING ) {
        p1 = (char *)response+1+bsp_response_strlen[response_t];
        if ( (p2 = strchr(p1, '\r') ) == NULL )
            return BSC_RES_UNRECOGNIZED;

        if ( ( *tube_name = strndup(p1, p2-p1) ) == NULL )
            return BSC_RES_CLIENT_OUT_OF_MEMORY;
    }

    return response_t;
}

/*-----------------------------------------------------------------------------
 * consumer methods
 *-----------------------------------------------------------------------------*/


GEN_STATIC_CMD(reserve, "reserve\r\n")

char *bsp_gen_reserve_with_to_cmd(int *cmd_len, bool *is_allocated, uint32_t timeout)
{
    static const char   format[]  = "reserve-with-timeout %u\r\n";
    static const size_t alloc_len = CSTRLEN(format) - CSTRLEN("%u") + UINT32_STRL + 1;

    INIT_CMD_MALLOC(timeout)
}

bsc_response_t bsp_get_reserve_res(const char *response, uint64_t *id, size_t *bytes)
{
    static const bsc_response_t bsp_reserve_cmd_responses[] = {
        BSC_RESERVE_RES_RESERVED,
        BSC_RESERVE_RES_DEADLINE_SOON,
        BSC_RESERVE_RES_TIMED_OUT
    };

    bsc_response_t response_t;
    char *p = NULL;

    bsp_get_response_t(response, bsp_reserve_cmd_responses);
    if ( response_t == BSC_RESERVE_RES_RESERVED ) {
        GET_ID_BYTES
    }

    return response_t;
}

char *bsp_gen_delete_cmd(int *cmd_len, bool *is_allocated, uint64_t id)
{
    static const char   format[]  = "delete %llu\r\n";
    static const size_t alloc_len = CSTRLEN(format) - CSTRLEN("%llu") + UINT64_STRL + 1;

    INIT_CMD_MALLOC(id)
}

bsc_response_t bsp_get_delete_res(const char *response)
{
    static const bsc_response_t bsp_delete_cmd_responses[] = {
         BSC_DELETE_RES_DELETED,
         BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    bsp_get_response_t(response, bsp_delete_cmd_responses);
    return response_t;
}

char *bsp_gen_release_cmd(int *cmd_len, bool *is_allocated, uint64_t id, uint32_t priority, uint32_t delay)
{
    static const char   format[]  = "release %llu %u %u\r\n";
    static const size_t alloc_len = CSTRLEN(format) + UINT64_STRL - CSTRLEN("%llu") + (UINT32_STRL - CSTRLEN("%u")) * 2 + 1;

    INIT_CMD_MALLOC(id, priority, delay)
}

bsc_response_t bsp_get_release_res(const char *response)
{
    static const bsc_response_t bsp_release_cmd_responses[] = {
         BSC_RELEASE_RES_RELEASED,
         BSC_RES_BURIED,
         BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    bsp_get_response_t(response, bsp_release_cmd_responses);
    return response_t;
}

char *bsp_gen_bury_cmd(int *cmd_len, bool *is_allocated, uint64_t id, uint32_t priority)
{
    static const char   format[]  = "bury %llu %u\r\n";
    static const size_t alloc_len = CSTRLEN(format) + UINT64_STRL - CSTRLEN("%llu") + UINT32_STRL - CSTRLEN("%u") + 1;

    INIT_CMD_MALLOC(id, priority)
}

bsc_response_t bsp_get_bury_res(const char *response)
{
    static const bsc_response_t bsp_bury_cmd_responses[] = {
         BSC_RES_BURIED,
         BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    bsp_get_response_t(response, bsp_bury_cmd_responses);
    return response_t;
}

char *bsp_gen_touch_cmd(int *cmd_len, bool *is_allocated, uint64_t id)
{
    static const char   format[]  = "touch %llu\r\n";
    static const size_t alloc_len = CSTRLEN(format) + UINT64_STRL - CSTRLEN("%llu") + 1;

    INIT_CMD_MALLOC(id)
}

bsc_response_t bsp_get_touch_res(const char *response)
{
    static const bsc_response_t bsp_touch_cmd_responses[] = {
         BSC_TOUCH_RES_TOUCHED,
         BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    bsp_get_response_t(response, bsp_touch_cmd_responses);
    return response_t;
}

char *bsp_gen_watch_cmd(int *cmd_len, bool *is_allocated, const char *tube_name)
{
    static const char   format[]   = "watch %s\r\n";
    static const size_t format_len = CSTRLEN(format) - CSTRLEN("%s") + 1;

    size_t alloc_len = format_len + strlen(tube_name);
    INIT_CMD_MALLOC(tube_name)
}

bsc_response_t bsp_get_watch_res(const char *response, uint32_t *count)
{
    static const bsc_response_t bsp_watch_cmd_responses[] = {
        BSC_RES_WATCHING
    };

    bsc_response_t response_t;
    char *p = NULL;
    ssize_t matched = EOF;

    bsp_get_response_t(response, bsp_watch_cmd_responses);
    if ( response_t == BSC_RES_WATCHING ) {
        p =  (char *)response + bsp_response_strlen[response_t] + 1;
        matched = sscanf(p, "%u", count );

        // got bad response format
        if (matched == EOF)
            return BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

char *bsp_gen_ignore_cmd(int *cmd_len, bool *is_allocated, const char *tube_name)
{
    static const char   format[]   = "ignore %s\r\n";
    static const size_t format_len = CSTRLEN(format) - CSTRLEN("%s") + 1;

    size_t alloc_len = format_len + strlen(tube_name);
    INIT_CMD_MALLOC(tube_name)
}

bsc_response_t bsp_get_ignore_res(const char *response, uint32_t *count)
{
    static const bsc_response_t bsp_ignore_cmd_responses[] = {
        BSC_RES_WATCHING,
        BSC_IGNORE_RES_NOT_IGNORED
    };

    bsc_response_t response_t;
    char *p = NULL;
    ssize_t matched = EOF;

    bsp_get_response_t(response, bsp_ignore_cmd_responses);
    if ( response_t == BSC_RES_WATCHING ) {
        p =  (char *)response + bsp_response_strlen[response_t] + 1;
        matched = sscanf(p, "%u", count );

        // got bad response format
        if (matched == EOF)
            return BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

/*-----------------------------------------------------------------------------
 * other methods
 *-----------------------------------------------------------------------------*/

char *bsp_gen_peek_cmd(int *cmd_len, bool *is_allocated, uint64_t id)
{
    static const char   format[]  = "peek %llu\r\n";
    static const size_t alloc_len = CSTRLEN(format) + UINT64_STRL - CSTRLEN("%llu") + 1;

    INIT_CMD_MALLOC(id)
}

GEN_STATIC_CMD(peek_ready, "peek-ready\r\n")
GEN_STATIC_CMD(peek_delayed, "peek-delayed\r\n")
GEN_STATIC_CMD(peek_buried, "peek-buried\r\n")

bsc_response_t bsp_get_peek_res(const char *response, uint64_t *id, size_t *bytes)
{
    static const bsc_response_t bsp_peek_cmd_responses[] = {
        BSC_PEEK_RES_FOUND,
        BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    char *p = NULL;

    bsp_get_response_t(response, bsp_peek_cmd_responses);
    if ( response_t == BSC_PEEK_RES_FOUND ) {
        GET_ID_BYTES
    }

    return response_t;
}

char *bsp_gen_kick_cmd(int *cmd_len, bool *is_allocated, uint32_t bound)
{
    static const char   format[]  = "kick %u\r\n";
    static const size_t alloc_len = CSTRLEN(format) + UINT32_STRL - CSTRLEN("%u") + 1;

    INIT_CMD_MALLOC(bound)
}

bsc_response_t bsp_get_kick_res(const char *response, uint32_t *count)
{
    static const bsc_response_t bsp_kick_cmd_responses[] = {
        BSC_KICK_RES_KICKED
    };

    bsc_response_t response_t;
    char *p = NULL, *p_tmp = NULL;

    bsp_get_response_t(response, bsp_kick_cmd_responses);

    if ( response_t == BSC_KICK_RES_KICKED ) {
        p =  (char *)response + bsp_response_strlen[response_t] + 1;
        *count = strtoul(p, &p_tmp, 10);
        if ( ( p = p_tmp ) == NULL)
            return BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

GEN_STATIC_CMD(quit, "quit\r\n")

char *bsp_gen_pause_tube_cmd(int *cmd_len, bool *is_allocated, const char *tube_name, uint32_t delay)
{
    static const char   format[]   = "pause-tube %s %u\r\n";
    static const size_t format_len = CSTRLEN(format) - CSTRLEN("%s") + UINT32_STRL - CSTRLEN("%u") + 1;

    size_t alloc_len = format_len + strlen(tube_name);
    INIT_CMD_MALLOC(tube_name, delay)
}

bsc_response_t bsp_get_pause_tube_res(const char *response)
{
    static const bsc_response_t bsp_pause_tube_cmd_responses[] = {
        BSC_PAUSE_TUBE_RES_PAUSED,
        BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    bsp_get_response_t(response, bsp_pause_tube_cmd_responses);
    return response_t;
}

/*-----------------------------------------------------------------------------
 * stats
 *-----------------------------------------------------------------------------*/

#define get_int32_from_yaml(var)\
    var = strtoul(p, &p_tmp, 10 );\
    if ( ( p = p_tmp + 3 + key_len[curr_key++] ) != NULL )\
        p_tmp = NULL;\
    else\
        goto parse_error;

#define get_int64_from_yaml(var)\
    var = strtoull(p, &p_tmp, 10 );\
    if ( ( p = p_tmp + 3 + key_len[curr_key++] ) != NULL )\
        p_tmp = NULL;\
    else\
        goto parse_error;

#define get_dbl_from_yaml(var)\
    var = strtod(p, &p_tmp);\
    if ( ( p = p_tmp + 3 + key_len[curr_key++] ) != NULL )\
        p_tmp = NULL;\
    else\
        goto parse_error;

#define get_str_from_yaml(var)\
    p_tmp = strchr(p, '\n');\
    if (p_tmp == NULL)\
        goto parse_error;\
    len = p_tmp-p;\
    if ( ( var = strndup(p, len) ) == NULL )\
        goto parse_error;\
    var[len] = '\0';\
    p = p_tmp + 3 + key_len[curr_key++];\
    p_tmp = NULL;

/*-----------------------------------------------------------------------------
 * job stats
 *-----------------------------------------------------------------------------*/

char *bsp_gen_stats_job_cmd(size_t *cmd_len, bool *is_allocated, uint64_t id)
{
    static const char   format[]  = "stats-job %llu\r\n";
    static const size_t alloc_len = CSTRLEN(format) + UINT64_STRL - CSTRLEN("%llu") + 1;

    INIT_CMD_MALLOC(id)
}

bsc_response_t bsp_get_stats_job_res(const char *response, size_t *bytes)
{
    static const bsc_response_t bsp_stats_job_cmd_responses[] = {
        BSC_RES_OK,
        BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    char *p = NULL;

    bsp_get_response_t(response, bsp_stats_job_cmd_responses);

    if (response_t == BSC_RES_OK) {
        *bytes = strtoul(response + bsp_response_strlen[response_t] + 1, &p, 10);
        if ( p == NULL)
            response_t = BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

bsc_job_stats *bsp_parse_job_stats(const char *data)
{
    static const size_t key_len[] = {
        CSTRLEN("id"),
        CSTRLEN("tube"),
        CSTRLEN("state"),
        CSTRLEN("pri"),
        CSTRLEN("age"),
        CSTRLEN("delay"),
        CSTRLEN("ttr"),
        CSTRLEN("time_left"),
        CSTRLEN("reserves"),
        CSTRLEN("timeouts"),
        CSTRLEN("releases"),
        CSTRLEN("buries"),
        CSTRLEN("kicks")
    };

    static const char *job_state_str[] = {
        "ready",
        "buried",
        "reserved",
        "delayed"
    };

    bsc_job_stats *job;
    char *p = NULL, *p_tmp = NULL;
    int curr_key = 0, i;
    size_t len;

    if ( ( job = (bsc_job_stats *)malloc( sizeof(bsc_job_stats) ) ) == NULL )
        return NULL;

    char *state = NULL;
    job->tube = NULL;

    p = (char *)data + 5 + key_len[curr_key++];

    get_int64_from_yaml(job->id);
    get_str_from_yaml(job->tube);
    get_str_from_yaml(state);
    get_int32_from_yaml(job->pri);
    get_int32_from_yaml(job->age);
    get_int32_from_yaml(job->delay);
    get_int32_from_yaml(job->ttr);
    get_int32_from_yaml(job->time_left);
    get_int32_from_yaml(job->reserves);
    get_int32_from_yaml(job->timeouts);
    get_int32_from_yaml(job->releases);
    get_int32_from_yaml(job->buries);
    get_int32_from_yaml(job->kicks);

    job->state = BSC_JOB_STATE_UNKNOWN;
    for ( i = 0; i < sizeof(job_state_str) / sizeof(char *); ++i )
        if ( ( strcmp(state, job_state_str[i]) ) == 0 ) {
            job->state = (bsc_job_state)i;
            break;
        }

    free(state);
    return job;

parse_error:
    if (state != NULL)
        free(state);
    if (job->tube != NULL)
        free(job->tube);
    free(job);
    return NULL;
}

void bsc_job_stats_free(bsc_job_stats *job)
{
    free(job->tube);
    free(job);
}

/*-----------------------------------------------------------------------------
 * tube stats
 *-----------------------------------------------------------------------------*/

char *bsp_gen_stats_tube_cmd(int *cmd_len, bool *is_allocated, const char *tube_name)
{
    static const char   format[]  = "stats-tube %s\r\n";
    static const size_t format_len = CSTRLEN(format) - CSTRLEN("%s") + 1;

    size_t alloc_len = format_len + strlen(tube_name);
    INIT_CMD_MALLOC(tube_name)
}

bsc_response_t bsp_get_stats_tube_res(const char *response, size_t *bytes)
{
    static const bsc_response_t bsp_stats_tube_cmd_responses[] = {
        BSC_RES_OK,
        BSC_RES_NOT_FOUND
    };

    bsc_response_t response_t;
    char *p = NULL;

    bsp_get_response_t(response, bsp_stats_tube_cmd_responses);

    if (response_t == BSC_RES_OK) {
        *bytes = strtoul(response + bsp_response_strlen[response_t] + 1, &p, 10);
        if ( p == NULL)
            response_t = BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

bsc_tube_stats *bsp_parse_tube_stats(const char *data)
{
    static const size_t key_len[] = {
        CSTRLEN("name"),
        CSTRLEN("current_jobs_urgent"),
        CSTRLEN("current_jobs_ready"),
        CSTRLEN("current_jobs_reserved"),
        CSTRLEN("current_jobs_delayed"),
        CSTRLEN("current_jobs_buried"),
        CSTRLEN("total_jobs"),
        CSTRLEN("current_using"),
        CSTRLEN("current_watching"),
        CSTRLEN("current_waiting"),
        CSTRLEN("cmd_pause_tube"),
        CSTRLEN("pause"),
        CSTRLEN("pause_time_left")
    };

    bsc_tube_stats *tube;
    char *p = NULL, *p_tmp = NULL;
    int curr_key = 0;
    size_t len;

    if ( ( tube = (bsc_tube_stats *)malloc( sizeof(bsc_tube_stats) ) ) == NULL )
        return NULL;

    tube->name = NULL;

    p = (char *)data + 6 + key_len[curr_key++];

    get_str_from_yaml(tube->name);
    get_int32_from_yaml(tube->current_jobs_urgent);
    get_int32_from_yaml(tube->current_jobs_ready);
    get_int32_from_yaml(tube->current_jobs_reserved);
    get_int32_from_yaml(tube->current_jobs_delayed);
    get_int32_from_yaml(tube->current_jobs_buried);
    get_int32_from_yaml(tube->total_jobs);
    get_int32_from_yaml(tube->current_using);
    get_int32_from_yaml(tube->current_watching);
    get_int32_from_yaml(tube->current_waiting);
    get_int32_from_yaml(tube->cmd_pause_tube);
    get_int32_from_yaml(tube->pause);
    get_int32_from_yaml(tube->pause_time_left);

    return tube;

parse_error:
    if (tube->name != NULL)
        free(tube->name);
    free(tube);
    return NULL;
}

void bsc_tube_stats_free(bsc_tube_stats *tube)
{
    free(tube->name);
    free(tube);
}

GEN_STATIC_CMD(stats, "stats\r\n")

bsc_response_t bsp_get_stats_res(const char *response, size_t *bytes)
{
    static const bsc_response_t bsp_stats_cmd_responses[] = {
        BSC_RES_OK
    };

    bsc_response_t response_t;
    char *p = NULL;

    bsp_get_response_t(response, bsp_stats_cmd_responses);

    if (response_t == BSC_RES_OK) {
        *bytes = strtoul(response + bsp_response_strlen[response_t] + 1, &p, 10);
        if ( p == NULL)
            response_t = BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

bsc_server_stats *bsp_parse_server_stats(const char *data)
{
    static const size_t key_len[] = {
        CSTRLEN("current_jobs_urgent"),
        CSTRLEN("current_jobs_ready"),
        CSTRLEN("current_jobs_reserved"),
        CSTRLEN("current_jobs_delayed"),
        CSTRLEN("current_jobs_buried"),
        CSTRLEN("cmd_put"),
        CSTRLEN("cmd_peek"),
        CSTRLEN("cmd_peek_ready"),
        CSTRLEN("cmd_peek_delayed"),
        CSTRLEN("cmd_peek_buried"),
        CSTRLEN("cmd_reserve"),
        CSTRLEN("cmd_reserve_with_timeout"),
        CSTRLEN("cmd_delete"),
        CSTRLEN("cmd_release"),
        CSTRLEN("cmd_use"),
        CSTRLEN("cmd_watch"),
        CSTRLEN("cmd_ignore"),
        CSTRLEN("cmd_bury"),
        CSTRLEN("cmd_kick"),
        CSTRLEN("cmd_touch"),
        CSTRLEN("cmd_stats"),
        CSTRLEN("cmd_stats_job"),
        CSTRLEN("cmd_stats_tube"),
        CSTRLEN("cmd_list_tubes"),
        CSTRLEN("cmd_list_tube_used"),
        CSTRLEN("cmd_list_tubes_watched"),
        CSTRLEN("cmd_pause_tube"),
        CSTRLEN("job_timeouts"),
        CSTRLEN("total_jobs"),
        CSTRLEN("max_job_size"),
        CSTRLEN("current_tubes"),
        CSTRLEN("current_connections"),
        CSTRLEN("current_producers"),
        CSTRLEN("current_workers"),
        CSTRLEN("current_waiting"),
        CSTRLEN("total_connections"),
        CSTRLEN("pid"),
        CSTRLEN("version"),
        CSTRLEN("rusage_utime"),
        CSTRLEN("rusage_stime"),
        CSTRLEN("uptime"),
        CSTRLEN("binlog_oldest_index"),
        CSTRLEN("binlog_current_index"),
        CSTRLEN("binlog_max_size")
    };

    bsc_server_stats *server;
    char *p = NULL, *p_tmp = NULL;
    int curr_key = 0;
    size_t len;

    if ( ( server = (bsc_server_stats *)malloc( sizeof(bsc_server_stats) ) ) == NULL )
        return NULL;

    p = (char *)data + 5 + key_len[curr_key++];

    get_int32_from_yaml(server->current_jobs_urgent);
    get_int32_from_yaml(server->current_jobs_ready);
    get_int32_from_yaml(server->current_jobs_reserved);
    get_int32_from_yaml(server->current_jobs_delayed);
    get_int32_from_yaml(server->current_jobs_buried);
    get_int32_from_yaml(server->cmd_put);
    get_int32_from_yaml(server->cmd_peek);
    get_int32_from_yaml(server->cmd_peek_ready);
    get_int32_from_yaml(server->cmd_peek_delayed);
    get_int32_from_yaml(server->cmd_peek_buried);
    get_int32_from_yaml(server->cmd_reserve);
    get_int32_from_yaml(server->cmd_reserve_with_timeout);
    get_int32_from_yaml(server->cmd_delete);
    get_int32_from_yaml(server->cmd_release);
    get_int32_from_yaml(server->cmd_use);
    get_int32_from_yaml(server->cmd_watch);
    get_int32_from_yaml(server->cmd_ignore);
    get_int32_from_yaml(server->cmd_bury);
    get_int32_from_yaml(server->cmd_kick);
    get_int32_from_yaml(server->cmd_touch);
    get_int32_from_yaml(server->cmd_stats);
    get_int32_from_yaml(server->cmd_stats_job);
    get_int32_from_yaml(server->cmd_stats_tube);
    get_int32_from_yaml(server->cmd_list_tubes);
    get_int32_from_yaml(server->cmd_list_tube_used);
    get_int32_from_yaml(server->cmd_list_tubes_watched);
    get_int32_from_yaml(server->cmd_pause_tube);
    get_int32_from_yaml(server->job_timeouts);
    get_int32_from_yaml(server->total_jobs);
    get_int32_from_yaml(server->max_job_size);
    get_int32_from_yaml(server->current_tubes);
    get_int32_from_yaml(server->current_connections);
    get_int32_from_yaml(server->current_producers);
    get_int32_from_yaml(server->current_workers);
    get_int32_from_yaml(server->current_waiting);
    get_int32_from_yaml(server->total_connections);
    get_int32_from_yaml(server->pid);
    get_str_from_yaml(server->version);
    get_dbl_from_yaml(server->rusage_utime);
    get_dbl_from_yaml(server->rusage_stime);
    get_int32_from_yaml(server->uptime);
    get_int32_from_yaml(server->binlog_oldest_index);
    get_int32_from_yaml(server->binlog_current_index);
    get_int32_from_yaml(server->binlog_max_size);

    return server;

parse_error:
    free(server);
    return NULL;
}

void bsc_server_stats_free(bsc_server_stats *server)
{
    free(server);
}

GEN_STATIC_CMD(list_tubes, "list-tubes\r\n")
GEN_STATIC_CMD(list_tubes_watched, "list-tubes-watched\r\n")

bsc_response_t bsp_get_list_tubes_res(const char *response, size_t *bytes)
{
    static const bsc_response_t bsp_list_tubes_cmd_responses[] = {
        BSC_RES_OK
    };

    bsc_response_t response_t;
    char *p = NULL;

    bsp_get_response_t(response, bsp_list_tubes_cmd_responses);

    if (response_t == BSC_RES_OK) {
        *bytes = strtoul(response + bsp_response_strlen[response_t] + 1, &p, 10);
        if ( p == NULL)
            response_t = BSC_RES_UNRECOGNIZED;
    }

    return response_t;
}

char **bsp_parse_tube_list(const char *data)
{
    char     **tube_list, *tubes, *tubes_p, *p1 = NULL, *p2 = NULL;
    register size_t num_tubes, i, tube_strlen, total_tubes_strlen = 0;

    // skip the yaml "---\n" header
    p1 = (char *)data + 4;

    for ( num_tubes = 0; ( p2 = strchr(p1, '\n') ) != NULL; ++num_tubes ) {
        total_tubes_strlen += p2 - p1 - 1; // w/o(-)
        p1 = p2 + 1; // skip \n
    }

    num_tubes--;

    // skip the yaml "---\n" header and the first list "- "
    p1 = (char *)data + 6;

    if ( ( tube_list = (char **)malloc( sizeof(char*) * (num_tubes +1) ) ) == NULL )
        return NULL;

    if ( ( tubes = (char *)malloc( sizeof(char) * (total_tubes_strlen+1) ) ) == NULL ){
        free(tube_list);
        return NULL;
    }

    for ( i = 0; i != num_tubes; ++i ) {
        p2 = strchr(p1, '\n');
        tube_strlen = p2 - p1;
        tube_list[i] = tubes;
        memcpy(tube_list[i], p1, tube_strlen);
        tube_list[i][tube_strlen] = '\0';
        p1 = p2 + 3; // skip "\n- "
        tubes += tube_strlen + 1;
    }

    // add the final "\0" element to the list
    tube_list[i] = tubes;
    tube_list[i][0] = '\0';

    return tube_list;
}

/*
 * template for creating strlen arrays
    static const char const *keys[] = {
        "name",
        "current-jobs-urgent",
        "current-jobs-ready",
        "current-jobs-reserved",
        "current-jobs-delayed",
        "current-jobs-buried",
        "total-jobs",
        "current-using",
        "current-watching",
        "current-waiting",
        "cmd-pause-tube",
        "pause",
        "pause-time-left"
    };

    int i;
    printf("static const size_t key_len[] = {\n    ");
    for ( i = 0; i < sizeof(keys)/sizeof(char *); ++i )
        printf("%d, ", strlen(keys[i]) );
    printf("\n};\n");
*/

#ifdef __cplusplus
}
#endif
