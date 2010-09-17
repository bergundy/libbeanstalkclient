/**
 * =====================================================================================
 * @file     beanstalkproto.h
 * @brief    header file for beanstalkproto.c - beanstalk protocol related functions
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

#ifndef BEANSTALKPROTO_H
#define BEANSTALKPROTO_H 

#ifdef __cplusplus
	extern "C" {
#endif

#include "beanstalkclient.h"

#define  CRLF "\r\n"

/*-----------------------------------------------------------------------------
 * producer methods
 *-----------------------------------------------------------------------------*/

/** 
* generates a put command header
* 
* @param hdr_len      pointer to store length of the generated hdr
* @param is_allocated pointer to store weather the generated string is to be freed
* @param priority     the job's priority
* @param delay        the job's start delay
* @param ttr          the job's time to run
* @param bytes        the job's data length
* 
* @return the serialized header
*/
char *bsp_gen_put_hdr(int       *hdr_len,
                      bool      *is_allocated,
                      uint32_t   priority,
                      uint32_t   delay,
                      uint32_t   ttr,
                      size_t     bytes);

/** 
* parses response from the put command
* 
* @param response the response message
* @param id       a pointer to store the received id (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_put_res(const char *response, uint64_t *id);

/** 
* generates a use command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param tube_name  the tube name..
* 
* @return the serialized command
*/
char *bsp_gen_use_cmd(int *cmd_len, bool *is_allocated, const char *tube_name);

/** 
* parses response from the use command
* 
* @param response  the response message
* @param tube_name a pointer to store the received tube (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_use_res(const char *response, char **tube_name);

/*-----------------------------------------------------------------------------
 * consumer methods
 *-----------------------------------------------------------------------------*/

/** 
* generates a reserve command
* 
* @param cmd_len     pointer to store the generated command's length in
* @param is_allocated  pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_reserve_cmd(int *cmd_len, bool *is_allocated);

/** 
* generates a reserve with timeout command
* 
* @param cmd_len     pointer to store the generated command's length in
* @param is_allocated  pointer to store weather the generated string is to be freed
* @param timeout     timeout in seconds
* 
* @return the serialized command
*/
char *bsp_gen_reserve_with_to_cmd(int *cmd_len, bool *is_allocated, uint32_t timeout);

/** 
* parses a response to the reserve command
* 
* @param response the response message
* @param id       a pointer to store the received id (if available)
* @param bytes    a pointer to store the received amount of bytes (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_reserve_res(const char *response, uint64_t *id, size_t *bytes);

/** 
* generates a delete command
* 
* @param cmd_len     pointer to store the generated command's length in
* @param is_allocated  pointer to store weather the generated string is to be freed
* @param id          the to be deleted job's id
* 
* @return the serialized command
*/
char *bsp_gen_delete_cmd(int *cmd_len, bool *is_allocated, uint64_t id);

/** 
* parses a response to the delete command
* 
* @param response the response message
* 
* @return the response code
*/
bsc_response_t bsp_get_delete_res(const char *response);

/** 
* generates a release command
* 
* @param cmd_len     pointer to store the generated command's length in
* @param is_allocated  pointer to store weather the generated string is to be freed
* @param id          the to be released job's id
* @param priority
* @param delay
* 
* @return the serialized command
*/
char *bsp_gen_release_cmd(int *cmd_len, bool *is_allocated, uint64_t id, uint32_t priority, uint32_t delay);

/** 
* parses a response to the release command
* 
* @param response the response message
* 
* @return the response code
*/
bsc_response_t bsp_get_release_res(const char *response);

/** 
* generates a bury command
* 
* @param cmd_len     pointer to store the generated command's length in
* @param is_allocated  pointer to store weather the generated string is to be freed
* @param id          the to be buried job's id
* @param priority    a new priority to assign to the job
* 
* @return the serialized command
*/
char *bsp_gen_bury_cmd(int *cmd_len, bool *is_allocated, uint64_t id, uint32_t priority);

/** 
* parses a response to the bury command
* 
* @param response the response message
* 
* @return the response code
*/
bsc_response_t bsp_get_bury_res(const char *response);

/** 
* generates a touch command
* 
* @param cmd_len     pointer to store the generated command's length in
* @param is_allocated  pointer to store weather the generated string is to be freed
* @param id          the to be touched job's id
* 
* @return the serialized command
*/
char *bsp_gen_touch_cmd(int *cmd_len, bool *is_allocated, uint64_t id);

/** 
* parses a response to the touch command
* 
* @param response the response message
* 
* @return the response code
*/
bsc_response_t bsp_get_touch_res(const char *response);

/** 
* generates a watch command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param tube_name  the tube name
* 
* @return the serialized command
*/
char *bsp_gen_watch_cmd(int *cmd_len, bool *is_allocated, const char *tube_name);

/** 
* parses a response to the watch command
* 
* @param response the response message
* @param count    a pointer to store the tubes watched count
* 
* @return the response code
*/
bsc_response_t bsp_get_watch_res(const char *response, uint32_t *count);

/** 
* generates an ignore command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param tube_name  the tube name
* 
* @return the serialized command
*/
char *bsp_gen_ignore_cmd(int *cmd_len, bool *is_allocated, const char *tube_name);

/** 
* parses a response to the ignore command
* 
* @param response the response message
* @param count    a pointer to store the tubes watched count
* 
* @return the response code
*/
bsc_response_t bsp_get_ignore_res(const char *response, uint32_t *count);

/*-----------------------------------------------------------------------------
 * other methods
 *-----------------------------------------------------------------------------*/

/** 
* generates a peek command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param id         the id of the job to be peeked
* 
* @return the serialized command
*/
char *bsp_gen_peek_cmd(int *cmd_len, bool *is_allocated, uint64_t id);

/** 
* generates a peek-ready command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_peek_ready_cmd(int *cmd_len, bool *is_allocated);

/** 
* generates a peek-delayed command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_peek_delayed_cmd(int *cmd_len, bool *is_allocated);

/** 
* generates a peek-buried command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_peek_buried_cmd(int *cmd_len, bool *is_allocated);

/** 
* parses a response to the peek command
* 
* @param response the response message
* @param id       a pointer to store the received id (if available)
* @param bytes    a pointer to store the received amount of bytes (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_peek_res(const char *response, uint64_t *id, size_t *bytes);

/** 
* generates a kick command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param bound      maximum jobs to kick
* 
* @return the serialized command
*/
char *bsp_gen_kick_cmd(int *cmd_len, bool *is_allocated, uint32_t bound);

/** 
* parses a response to the kick command
* 
* @param response the response message
* @param count    a pointer to store the kicked count (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_kick_res(const char *response, uint32_t *count);

/** 
* generates a quit command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_quit_cmd(int *cmd_len, bool *is_allocated);

/** 
* generates a pause-tube command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param tube_name  the tube's name
* @param delay      in seconds
* 
* @return the serialized command
*/
char *bsp_gen_pause_tube_cmd(int *cmd_len, bool *is_allocated, const char *tube_name, uint32_t delay);

/** 
* parses a response to the pause-tube command
* 
* @param response the response message
* 
* @return the response code
*/
bsc_response_t bsp_get_pause_tube_res(const char *response);

/*-----------------------------------------------------------------------------
 * job stats
 *-----------------------------------------------------------------------------*/

/** 
* generates a stats-job command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param id         the id of the job to be queried
* 
* @return the serialized command
*/
char *bsp_gen_stats_job_cmd(size_t *cmd_len, bool *is_allocated, uint64_t id);

/** 
* parses a response to the stats-job command
* 
* @param response the response message
* @param bytes    a pointer to store the received amount of bytes (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_stats_job_res(const char *response, size_t *bytes);

/** 
* parses the job stats yaml
* 
* @param data the yaml
* 
* @return a job stats struct pointer
*/
bsc_job_stats *bsp_parse_job_stats(const char *data);

/*-----------------------------------------------------------------------------
 * tube stats
 *-----------------------------------------------------------------------------*/

/** 
* generates a stats-tube command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* @param tube_name  the tube's name
* 
* @return the serialized command
*/
char *bsp_gen_stats_tube_cmd(int *cmd_len, bool *is_allocated, const char *tube_name);

/** 
* parses a response to the stats-tube command
* 
* @param response the response message
* @param bytes    a pointer to store the received amount of bytes (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_stats_tube_res(const char *response, size_t *bytes);

/** 
* parses the tube stats yaml
* 
* @param data the yaml
* 
* @return a tube stats struct pointer
*/
bsc_tube_stats *bsp_parse_tube_stats(const char *data);

/*-----------------------------------------------------------------------------
 * server stats
 *-----------------------------------------------------------------------------*/

/** 
* generates a stats command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_stats_cmd(int *cmd_len, bool *is_allocated);

/** 
* parses a response to the stats command
* 
* @param response the response message
* @param bytes    a pointer to store the received amount of bytes (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_stats_res(const char *response, size_t *bytes);

/** 
* parses the server stats yaml
* 
* @param data the yaml
* 
* @return a server stats struct pointer
*/
bsc_server_stats *bsp_parse_server_stats(const char *data);

/*-----------------------------------------------------------------------------
 * list tubes
 *-----------------------------------------------------------------------------*/

/** 
* generates a list tubes command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_list_tubes_cmd(int *cmd_len, bool *is_allocated);

/** 
* generates a list tubes watched command
* 
* @param cmd_len    pointer to store the generated command's length in
* @param is_allocated pointer to store weather the generated string is to be freed
* 
* @return the serialized command
*/
char *bsp_gen_list_tubes_watched_cmd(int *cmd_len, bool *is_allocated);

/** 
* parses a response to the list-tubes command
* 
* @param response the response message
* @param bytes    a pointer to store the received amount of bytes (if available)
* 
* @return the response code
*/
bsc_response_t bsp_get_list_tubes_res(const char *response, size_t *bytes);

/** 
* parses the server stats yaml
* 
* @param data the yaml
* 
* @return a NULL terminated array of tubes (strings)
*/
char **bsp_parse_tube_list(const char *data);

#ifdef __cplusplus
}
#endif

#endif /* BEANSTALKPROTO_H */
