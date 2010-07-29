/**
 * =====================================================================================
 * @file   check_bsc.c
 * @brief  test suite for beanstalkclient library
 * @date   06/13/2010 05:58:28 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include "beanstalkclient.h"

bsc *client;
char *host = "localhost", *port = BSP_DEFAULT_PORT;
char *reconnect_test_port = "16666";
int counter = 0;
int finished = 0;
char *exp_data;
static char spawn_cmd[200];
static char kill_cmd[200];
static bsc_error_t bsc_error;

/* generic error handler - fail on error */
void onerror(bsc *client, bsc_error_t error)
{
    fail("bsc got error");
}

/*****************************************************************************************************************/ 
/*                                                      test 1                                                   */
/*****************************************************************************************************************/ 
void small_vec_cb(bsc *client, queue_node *node, void *data, size_t len);
void put_cb(bsc *client, struct bsc_put_info *info);
void reserve_cb(bsc *client, struct bsc_reserve_info *info);
void delete_cb(bsc *client, struct bsc_delete_info *info);

void put_cb(bsc *client, struct bsc_put_info *info)
{
    ++counter;
    fail_if(info->response.code != BSP_PUT_RES_INSERTED, "put_cb: info->code != BSP_PUT_RES_INSERTED");
}

void reserve_cb(bsc *client, struct bsc_reserve_info *info)
{
    ++counter;
    fail_if(info->response.code != BSP_RESERVE_RES_RESERVED,
        "bsp_reserve: response.code != BSP_RESERVE_RES_RESERVED");
    fail_if(info->response.bytes != strlen(exp_data),
        "bsp_reserve: response.bytes != exp_bytes");
    fail_if(strcmp(info->response.data, exp_data) != 0,
        "bsp_reserve: got invalid data");

    fail_if((bsc_error = bsc_delete(client, delete_cb, NULL, info->response.id)),
        "bsc_delete failed (%d)", bsc_error);
}

void delete_cb(bsc *client, struct bsc_delete_info *info)
{
    fail_if( info->response.code != BSP_DELETE_RES_DELETED,
        "bsp_delete: response.code != BSP_DELETE_RES_DELETED");

    if (++counter == 10) {
        exp_data = "bababuba12341234";
        fail_if((bsc_error = bsc_put(client, put_cb, NULL, 1, 0, 10, strlen(exp_data), exp_data, false)),
            "BSC_ENQ_CMD failed (%d)", bsc_error);
        fail_if((bsc_error = bsc_reserve(client, reserve_cb, NULL, -1)),
            "bsc_reserve failed (%d)", bsc_error);
    }
    else
        ++finished;
}

void small_vec_cb(bsc *client, queue_node *node, void *data, size_t len)
{
    bsp_response_t res;
    switch (counter) {
        case 0:
            fail_if(strcmp(data, "USING test\r\n") != 0, "use cmd res");
            break;
        case 1:
            fail_if(strcmp(data, "WATCHING 2\r\n") != 0, "watch cmd res");
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            fail_if(strcmp(data, "WATCHING 1\r\n") != 0, "watch cmd res");
            break;
    }
    ++counter;
    fail_if( len != strlen(data), "got invalid len argument" );
}

START_TEST(test_bsc_small_vec) {
    char *errstr = NULL;
    client = bsc_new(host, port, onerror, 14, 12, 4, &errstr);
    fail_if( client == NULL, "bsc_new: %s", errstr);
    exp_data = "baba";
    fd_set readset, writeset;
    int i;

    bsc_error = BSC_ENQ_CMD(client, use,     small_vec_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, watch,   small_vec_cb, NULL, "test");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore,  small_vec_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore,  small_vec_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore,  small_vec_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore,  small_vec_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore,  small_vec_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore,  NULL, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = bsc_put(client, put_cb, NULL, 1, 0, 10, strlen(exp_data), exp_data, false);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_put failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, reserve_cb, NULL, -1);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!finished) {
        FD_SET(client->fd, &readset);
        FD_SET(client->fd, &writeset);
        if (AQUEUE_EMPTY(client->outq)) {
            if ( select(client->fd+1, &readset, NULL, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(client->fd, &readset)) {
                bsc_read(client);
            }
        }
        else {
            if ( select(client->fd+1, &readset, &writeset, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(client->fd, &readset)) {
                bsc_read(client);
            }
            if (FD_ISSET(client->fd, &writeset)) {
                bsc_write(client);
            }
        }
    }

    return;

cmd_error:
    bsc_free(client);
    fail("error enqueuing command");
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                      test 2                                                   */
/*****************************************************************************************************************/ 

void fin_cb(bsc *client, queue_node *node, void *data, size_t len)
{
    if (strcmp(node->cb_data->user_data, exp_data) != 0)
        fail("got invalid data");
    ++finished;
}

START_TEST(test_bsc_defaults) {
    char *errstr = NULL;
    fd_set readset, writeset;
    int i;
    exp_data = "baba";
    client = bsc_new_w_defaults(host, port, onerror, &errstr);
    fail_if( client == NULL, "bsc_new: %s", errstr);

    bsc_error = BSC_ENQ_CMD(client, ignore, NULL, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore, NULL, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore, NULL, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = BSC_ENQ_CMD(client, ignore, fin_cb, exp_data, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(client->fd, &readset);
    FD_SET(client->fd, &writeset);

    while (!finished) {
        FD_SET(client->fd, &readset);
        FD_SET(client->fd, &writeset);
        if (AQUEUE_EMPTY(client->outq)) {
            if ( select(client->fd+1, &readset, NULL, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(client->fd, &readset)) {
                bsc_read(client);
            }
        }
        else {
            if ( select(client->fd+1, &readset, &writeset, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(client->fd, &readset)) {
                bsc_read(client);
            }
            if (FD_ISSET(client->fd, &writeset)) {
                bsc_write(client);
            }
        }
    }

    bsc_free(client);
    return;
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                      test 3                                                   */
/*****************************************************************************************************************/ 

void reconnect_test_cb(bsc *client, queue_node *node, void *data, size_t len)
{
    system(kill_cmd);
}

static void reconnect(bsc *client, bsc_error_t error)
{
    int i;
    char *errorstr;
    system(spawn_cmd);

    if (error == BSC_ERROR_INTERNAL) {
        fail("critical error: recieved BSC_ERROR_INTERNAL, quitting\n");
    }
    else if (error == BSC_ERROR_SOCKET) {
        if ( bsc_reconnect(client, &errorstr) ) {
            fail_if( client->outq->used != 9, 
                "after reconnect: nodes_used : %d/%d", client->outq->used, 9);

            finished++;
            return;
        }
    }
    fail("critical error: maxed out reconnect attempts, quitting\n");
}

START_TEST(test_bsc_reconnect) {
    fd_set readset, writeset;
    char *errstr = NULL;
    sprintf(spawn_cmd, "beanstalkd -p %s -d", reconnect_test_port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", reconnect_test_port);
    system(spawn_cmd);
    client = bsc_new( host, reconnect_test_port, reconnect, 10, 12, 4, &errstr);
    fail_if( client == NULL, "bsc_new: %s", errstr);

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(client->fd, &readset);
    FD_SET(client->fd, &writeset);

    bsc_error = BSC_ENQ_CMD(client, ignore, reconnect_test_cb, NULL, "default");
    fail_if(bsc_error != BSC_ERROR_NONE, "BSC_ENQ_CMD failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, -1);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, -1);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    bsc_error = bsc_reserve(client, NULL, NULL, -1);
    fail_if(bsc_error = bsc_put(client, NULL, NULL, 1, 0, 10, strlen(kill_cmd), kill_cmd, false) != BSC_ERROR_NONE, 
        "bsc_put failed (%d)", bsc_error);
    fail_if(bsc_error != BSC_ERROR_NONE, "bsc_reserve failed (%d)", bsc_error);
    fail_if(bsc_error = bsc_put(client, NULL, NULL, 1, 0, 10, strlen(kill_cmd), kill_cmd, false) != BSC_ERROR_NONE, 
        "bsc_put failed (%d)", bsc_error);


    while (!finished) {
        FD_SET(client->fd, &readset);
        FD_SET(client->fd, &writeset);
        if (AQUEUE_EMPTY(client->outq)) {
            if ( select(client->fd+1, &readset, NULL, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(client->fd, &readset)) {
                bsc_read(client);
            }
        }
        else {
            if ( select(client->fd+1, &readset, &writeset, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(client->fd, &readset)) {
                bsc_read(client);
            }
            if (FD_ISSET(client->fd, &writeset)) {
                bsc_write(client);
            }
        }
    }

    system(kill_cmd);
    bsc_free(client);
    return;
}
END_TEST

/*****************************************************************************************************************/ 
/*                                                  end of tests                                                 */
/*****************************************************************************************************************/ 

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("bsc");

    tcase_add_test(tc, test_bsc_small_vec);
    tcase_add_test(tc, test_bsc_defaults);
    tcase_add_test(tc, test_bsc_reconnect);

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
