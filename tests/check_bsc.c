/**
 * =====================================================================================
 * @file     check_bsc.c
 * @brief    test suite for beanstalkclient library
 * @date     06/13/2010 05:58:28 PM
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

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include "beanstalkclient.h"

char *host = "localhost", *port = BSC_DEFAULT_PORT;
char *reconnect_test_port = "16666";
char *tube_test_port = "16667";
int counter = 0;
int finished = 0;
char *exp_data;
static char spawn_cmd[200];
static char kill_cmd[200];
static bsc_error_t bsc_error;

int client_poll(bsc *client, fd_set *readset, fd_set *writeset)
{
    FD_SET(client->fd, readset);
    FD_SET(client->fd, writeset);
    if (AQ_EMPTY(client->outq)) {
        if ( select(client->fd+1, readset, NULL, NULL, NULL) < 0) {
            fprintf(stderr, "critical error: select()");
            return EXIT_FAILURE;
        }
        if (FD_ISSET(client->fd, readset))
            bsc_read(client);
    }
    else {
        if ( select(client->fd+1, readset, writeset, NULL, NULL) < 0) {
            fprintf(stderr, "critical error: select()");
            return EXIT_FAILURE;
        }
        if (FD_ISSET(client->fd, readset))
            bsc_read(client);
        if (FD_ISSET(client->fd, writeset))
            bsc_write(client);
    }
}

/* generic error handler - fail on error */
void onerror(bsc *client, bsc_error_t error)
{
    fail("bsc got error");
}

/*****************************************************************************************************************/ 
/*                                                      test 1                                                   */
/*****************************************************************************************************************/ 
void put_cb(bsc *client, struct bsc_put_info *info);
void reserve_cb(bsc *client, struct bsc_reserve_info *info);
void delete_cb(bsc *client, struct bsc_delete_info *info);

void put_cb(bsc *client, struct bsc_put_info *info)
{
    fail_if(info->response.code != BSC_PUT_RES_INSERTED, "put_cb: info->code != BSC_PUT_RES_INSERTED");
}

void reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    fail_if(info->response.code != BSC_RESERVE_RES_RESERVED,
        "bsp_reserve: response.code != BSC_RESERVE_RES_RESERVED");
    fail_if(info->response.bytes != strlen(exp_data),
        "bsp_reserve: response.bytes != exp_bytes");
    fail_if(strcmp(info->response.data, exp_data) != 0,
        "bsp_reserve: got invalid data");

    bsc_error = bsc_delete(client, delete_cb, NULL, info->response.id);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_delete failed (%d)", bsc_error);
}

void delete_cb(bsc *client, struct bsc_delete_info *info)
{
    fail_if( info->response.code != BSC_DELETE_RES_DELETED,
        "bsp_delete: response.code != BSC_DELETE_RES_DELETED");

    if (info->user_data) {
        exp_data = "bababuba12341234";
        bsc_error = bsc_put(client, put_cb, client, 1, 0, 10, strlen(exp_data), exp_data, false);
        fail_if(bsc_error != BSC_ERROR_NONE, "bsc_put failed (%d)", bsc_error);
        bsc_error = bsc_reserve(client, reserve_cb, NULL, -1);
        fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    }
    else
        ++finished;
}

void use_cb(bsc *client, struct bsc_use_info *info)
{
    fail_if( info->response.code != BSC_USE_RES_USING,
        "bsp_use: response.code != BSC_USE_RES_USING");
    fail_if( strcmp(info->response.tube, info->request.tube),
        "bsp_use: response.tube != info->request.tube");
}

void watch_cb(bsc *client, struct bsc_watch_info *info)
{
    fail_if( info->response.code != BSC_RES_WATCHING,
        "bsp_watch: response.code != BSC_RES_WATCHING");
    fail_if( client->watched_tubes_count != info->response.count,
        "bsp_watch: watched_tubes_count != response.count" );
    fail_if( strcmp(client->watched_tubes->name, BSC_DEFAULT_TUBE) );
    fail_if( strcmp(client->watched_tubes->next->name, info->request.tube) );
}

void ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    fail_if( info->response.code != BSC_RES_WATCHING,
        "bsp_ignore: response.code != BSC_RES_WATCHING");
    fail_if( client->watched_tubes_count != info->response.count,
        "bsp_ignore: watched_tubes_count != response.count" );
    fail_if( strcmp(client->watched_tubes->name, "test") );
}


START_TEST(small_vec_test) {
    char errorstr[BSC_ERRSTR_LEN];
    bsc *client;
    client = bsc_new(host, port, BSC_DEFAULT_TUBE, onerror, 16, 12, 4, errorstr);
    fail_if( client == NULL, "bsc_new: %s", errorstr);
    exp_data = "baba";
    fd_set readset, writeset;

    bsc_error = bsc_use(client, use_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_use failed (%d)", bsc_error );

    bsc_error = bsc_watch(client, watch_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );

    bsc_error = bsc_watch(client, watch_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default"); 
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_put(client, put_cb, NULL, 1, 0, 10, strlen(exp_data), exp_data, false);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_put failed (%d)", bsc_error);

    bsc_error = bsc_reserve(client, reserve_cb, NULL, -1);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!finished) {
        if (client_poll(client, &readset, &writeset) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }

    bsc_free(client);
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                      test 2                                                   */
/*****************************************************************************************************************/ 

void ignore_cb2(bsc *client, struct bsc_ignore_info *info)
{
    fail_if( info->response.code != BSC_IGNORE_RES_NOT_IGNORED,
        "bsc_ignore: response.code != BSC_IGNORE_RES_NOT_IGNORED");
    fail_if( strcmp(client->watched_tubes->name, BSC_DEFAULT_TUBE) );

    if (info->user_data != NULL)
        ++finished;
}


START_TEST(commands_test) {
    bsc *client;
    char errorstr[BSC_ERRSTR_LEN];
    fd_set readset, writeset;
    exp_data = "baba";
    client = bsc_new_w_defaults(host, port, BSC_DEFAULT_TUBE, onerror, errorstr);
    fail_if( client == NULL, "bsc_new: %s", errorstr);

    bsc_error = bsc_ignore(client, ignore_cb2, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb2, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    bsc_error = bsc_ignore(client, ignore_cb2, client, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!finished) {
        if (client_poll(client, &readset, &writeset) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }

    bsc_free(client);
    return;
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                      test 3                                                   */
/*****************************************************************************************************************/ 
static bool put_finished = false;

void put_client_put_cb(bsc *client, struct bsc_put_info *info)
{
    put_finished = true;
}

void spawn_put_client()
{
    char errorstr[BSC_ERRSTR_LEN];
    fd_set readset, writeset;

    bsc *put_client;
    put_client = bsc_new(host, reconnect_test_port, "baba", NULL, 16, 12, 4, errorstr);
    if(!put_client) {
        fprintf(stderr, "bsc_new: %s", errorstr);
        exit(EXIT_FAILURE);
    }

    bsc_error = bsc_put(put_client, put_client_put_cb, NULL, 1, 0, 10, strlen("baba"), "baba", false);

    if (bsc_error != BSC_ERROR_NONE) {
        fprintf(stderr, "bsc_put failed (%d)", bsc_error );
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!put_finished)
        client_poll(put_client, &readset, &writeset);
    printf("put_client: put finished\n");
}


void reconnect_test_ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    system(kill_cmd);
}

void reconnect_test_reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    finished++;
}

static void reconnect(bsc *client, bsc_error_t error)
{
    char errorstr[BSC_ERRSTR_LEN];
    system(spawn_cmd);
    spawn_put_client();

    if (error == BSC_ERROR_INTERNAL) {
        fail("critical error: recieved BSC_ERROR_INTERNAL, quitting\n");
    }
    else if (error == BSC_ERROR_SOCKET) {
        if ( bsc_reconnect(client, errorstr) ) {
            fail_if( client->outq->used != 9, 
                "after reconnect: nodes_used : %d/%d", client->outq->used, 9);

            printf("reconnect successful\n", client);

            return;
        }
        fail("critical error: maxed out reconnect attempts, quitting\n");
    }
}

START_TEST(reconnect_test) {
    bsc *client;
    fd_set readset, writeset;
    char errorstr[BSC_ERRSTR_LEN];
    sprintf(spawn_cmd, "beanstalkd -p %s -d", reconnect_test_port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd|grep '%s'|gawk '!/grep/ {print $2}'|xargs kill", reconnect_test_port);
    system(spawn_cmd);
    client = bsc_new( host, reconnect_test_port, "baba", reconnect, 16, 12, 4, errorstr);
    fail_if( client == NULL, "bsc_new: %s", errorstr);

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(client->fd, &readset);
    FD_SET(client->fd, &writeset);

    bsc_error = bsc_ignore(client, reconnect_test_ignore_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );
    bsc_error = bsc_reserve(client, reconnect_test_reserve_cb, NULL, BSC_RESERVE_NO_TIMEOUT);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, BSC_RESERVE_NO_TIMEOUT);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, BSC_RESERVE_NO_TIMEOUT);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_put(client, NULL, NULL, 1, 0, 10, strlen(kill_cmd), kill_cmd, false);
    fail_if(bsc_error, "bsc_put failed (%d)", bsc_error);
    bsc_error = bsc_put(client, NULL, NULL, 1, 0, 10, strlen(kill_cmd), kill_cmd, false);
    fail_if(bsc_error, "bsc_put failed (%d)", bsc_error);

    while (!finished) {
        if (client_poll(client, &readset, &writeset) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }

    system(kill_cmd);
    bsc_free(client);
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                      test 4                                                   */
/*****************************************************************************************************************/ 

static int watch_cb_count = 0;
static int ignore_cb_count = 0;

void tube_test_watch_cb(bsc *client, struct bsc_watch_info *info);

void tube_test_ignore_cb2(bsc *client, struct bsc_ignore_info *info)
{
    fail_if(client->watched_tubes_count != 1, "watched tubes: %d/%d", client->watched_tubes_count, 2);
    fail_if(strcmp(client->watched_tubes->name, "baba1"), "watched tubes [0]: %s", client->watched_tubes->name);
    fail_if(client->watched_tubes->next, "watched tubes [1]: != NULL");

    bsc_error = bsc_watch(client, tube_test_watch_cb, NULL, "baba2");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );
}

void tube_test_ignore_cb(bsc *client, struct bsc_ignore_info *info)
{
    fail_if(client->watched_tubes_count != 2, "watched tubes: %d/%d", client->watched_tubes_count, 2);
    fail_if(strcmp(client->watched_tubes->name, "baba1"), "watched tubes [0]: %s", client->watched_tubes->name);
    fail_if(strcmp(client->watched_tubes->next->name, "baba2"), "watched tubes [1]: %s", client->watched_tubes->next->name);
    fail_if(client->watched_tubes->next->next, "watched tubes [2]: != NULL");

    if (!ignore_cb_count++) {
        bsc_error = bsc_ignore(client, tube_test_ignore_cb2, NULL, "baba2");
        fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );
    }
    else {
        system(kill_cmd);
    }
}

void tube_test_watch_cb2(bsc *client, struct bsc_watch_info *info)
{
    fail_if(client->watched_tubes_count != 3, "watched tubes: %d/%d", client->watched_tubes_count, 2);
    fail_if(strcmp(client->watched_tubes->name, "aba"), "watched tubes [0]: %s", client->watched_tubes->name);
    fail_if(strcmp(client->watched_tubes->next->name, "baba1"), "watched tubes [1]: %s", client->watched_tubes->next->name);
    fail_if(strcmp(client->watched_tubes->next->next->name, "baba2"), "watched tubes [2]: %s", client->watched_tubes->next->next->name);
    fail_if(client->watched_tubes->next->next->next, "watched tubes [3]: != NULL");

    bsc_error = bsc_ignore(client, tube_test_ignore_cb, NULL, "aba");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_ignore failed (%d)", bsc_error );
}

void tube_test_watch_cb(bsc *client, struct bsc_watch_info *info)
{
    fail_if(client->watched_tubes_count != 2, "watched tubes: %d/%d", client->watched_tubes_count, 2);
    fail_if(strcmp(client->watched_tubes->name, "baba1"), "watched tubes [0]: %s", client->watched_tubes->name);
    fail_if(strcmp(client->watched_tubes->next->name, "baba2"), "watched tubes [1]: %s", client->watched_tubes->next->name);
    fail_if(client->watched_tubes->next->next, "watched tubes [2]: != NULL");

    if (watch_cb_count++) {
        bsc_error = bsc_watch(client, tube_test_watch_cb2, NULL, "aba");
        fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );
    }
}

static void tt_reconnect(bsc *client, bsc_error_t error)
{
    char errorstr[BSC_ERRSTR_LEN];
    system(spawn_cmd);

    if (error == BSC_ERROR_INTERNAL) {
        fail("critical error: recieved BSC_ERROR_INTERNAL, quitting\n");
    }
    else if (error == BSC_ERROR_SOCKET) {
        if ( bsc_reconnect(client, errorstr) ) {
            fail_if( AQ_REAR(client->tubeq) == NULL, 
                "after reconnect: AQ_REAR(client->tubeq) == NULL");

            fail_if( strcmp(AQ_REAR(client->tubeq)->vec->iov_base, "use baba1\r\n"),
                "after reconnect: AQ_REAR(client->tubeq) iov_base (%s) != (%s)",
                AQ_REAR(client->tubeq)->vec->iov_base, "use baba1\r\n");
            AQ_DEQ_FIN(client->tubeq);

            fail_if( strcmp(AQ_REAR(client->tubeq)->vec->iov_base, "watch baba1\r\n"),
                "after reconnect: AQ_REAR(client->tubeq) iov_base (%s) != (%s)",
                AQ_REAR(client->tubeq)->vec->iov_base, "watch baba1\r\n");
            AQ_DEQ_FIN(client->tubeq);

            fail_if( strcmp(AQ_REAR(client->tubeq)->vec->iov_base, "watch baba2\r\n"),
                "after reconnect: AQ_REAR(client->tubeq) iov_base (%s) != (%s)",
                AQ_REAR(client->tubeq)->vec->iov_base, "watch baba2\r\n");
            AQ_DEQ_FIN(client->tubeq);

            fail_if( strcmp(AQ_REAR(client->tubeq)->vec->iov_base, "ignore default\r\n"),
                "after reconnect: AQ_REAR(client->tubeq) iov_base (%s) != (%s)",
                AQ_REAR(client->tubeq)->vec->iov_base, "ignore default\r\n");

            finished++;
            return;
        }
    }
    fail("critical error: maxed out reconnect attempts, quitting\n");
}

START_TEST(tube_test) {
    bsc *client;
    fd_set readset, writeset;
    char errorstr[BSC_ERRSTR_LEN];

    sprintf(spawn_cmd, "beanstalkd -p %s -d", tube_test_port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", tube_test_port);
    system(spawn_cmd);

    client = bsc_new_w_defaults( host, tube_test_port, "baba1", tt_reconnect, errorstr);
    fail_if( client == NULL, "bsc_new: %s", errorstr);

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    bsc_error = bsc_watch(client, tube_test_watch_cb, NULL, "baba2");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );

    bsc_error = bsc_watch(client, tube_test_watch_cb, NULL, "baba2");
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_watch failed (%d)", bsc_error );

    while (!finished) {
        if (client_poll(client, &readset, &writeset) == EXIT_FAILURE)
            return EXIT_FAILURE;
    }

    bsc_free(client);
    system(kill_cmd);
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                  end of tests                                                 */
/*****************************************************************************************************************/ 

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("bsc");

    tcase_add_test(tc, small_vec_test);
    tcase_add_test(tc, commands_test);
    tcase_add_test(tc, reconnect_test);
    tcase_add_test(tc, tube_test);

    suite_add_tcase(s, tc);
    return s;
}

int main() {
    SRunner *sr;
    Suite *s;
    int failed;

    s = local_suite();
    sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);

    failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
