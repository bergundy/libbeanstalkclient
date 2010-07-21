/**
 * =====================================================================================
 * @file   check_evvector.c
 * @brief  test suite for evvector functions
 * @date   06/13/2010 05:58:28 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evbsc.h"

evbsc *bsc;
char *host = "localhost", *port = BSC_DEFAULT_PORT;
char *reconnect_test_port = "16666";
struct ev_loop *loop;
int counter = 0;
int fail = 0;
char *exp_data;
static char spawn_cmd[200];
static char kill_cmd[200];

void small_vec_cb(evbsc *bsc, queue_node *node, void *data, size_t len)
{
    //printf("got data: '%s'\n", data);
    bsc_response_t res;
    static uint32_t exp_id;
    static uint32_t res_id, bytes;
    switch (counter) {
        case 0:
            fail += strcmp(data, "USING test\r\n");
            break;
        case 1:
            fail += strcmp(data, "WATCHING 2\r\n");
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            fail += strcmp(data, "WATCHING 1\r\n");
            break;
        case 7:
        case 11:
            res = bsc_get_put_res(data, &exp_id);
            if (res != BSC_PUT_RES_INSERTED)
                fail++;
            break;
        case 8:
        case 12:
            res = bsc_get_reserve_res(data, &res_id, &bytes);
            if (res != BSC_RESERVE_RES_RESERVED || bytes != strlen(exp_data))
                fail++;
            node->bytes_expected = bytes;
            break;
        case 9:
        case 13:
            fail += strcmp(data, exp_data);
            BSC_ENQ_CMD(delete, bsc, small_vec_cb, NULL, cmd_error, res_id );
            break;
        case 10:
        case 14:
            res = bsc_get_delete_res(data);
            if ( res != BSC_DELETE_RES_DELETED )
                ++fail;
            else if (counter == 10) {
                exp_data = "bababuba12341234";
                BSC_ENQ_CMD(put,     bsc, small_vec_cb, NULL, cmd_error, 1, 0, 10, strlen(exp_data), exp_data);
                BSC_ENQ_CMD(reserve, bsc, small_vec_cb, NULL, cmd_error );
                break;
            }
        default:
            ev_unloop(EV_A_ EVUNLOOP_ALL);
    }
    ++counter;
    if ( len != strlen(data) )
        fail = 1;
    if (fail)
        ev_unloop(EV_A_ EVUNLOOP_ALL);

    return;

cmd_error:
    ++fail;
    ev_unloop(EV_A_ EVUNLOOP_ALL);
}

void fin_cb(evbsc *bsc, queue_node *node, void *data, size_t len)
{
    if (strcmp(node->cb_data, exp_data) != 0)
        fail++;
    ev_unloop(bsc->loop, EVUNLOOP_ALL);
}

void onerror(evbsc *bsc, evbsc_error_t error)
{
    ++fail;
    fail("evbsc got error");
}

START_TEST(test_evbsc_small_vec) {
    loop = ev_default_loop(0);
    char *errstr = NULL;
    bsc = evbsc_new( loop, host, port, onerror, 11, 12, 4, &errstr);
    fail_if( bsc == NULL, "evbsc_new: %s", errstr);
    exp_data = "baba";

    BSC_ENQ_CMD(use,     bsc, small_vec_cb, NULL, cmd_error, "test");
    BSC_ENQ_CMD(watch,   bsc, small_vec_cb, NULL, cmd_error, "test");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, NULL, NULL, cmd_error, "default");
    BSC_ENQ_CMD(put,     bsc, small_vec_cb, NULL, cmd_error, 1, 0, 10, strlen(exp_data), exp_data);
    BSC_ENQ_CMD(reserve, bsc, small_vec_cb, NULL, cmd_error );

    ev_loop(loop, 0);
    fail_if(fail, "got invalid data");
    return;

cmd_error:
    evbsc_free(bsc);
    fail("cmd_error");
}
END_TEST

START_TEST(test_evbsc_defaults) {
    loop = ev_default_loop(0);
    char *errstr = NULL;
    exp_data = "baba";
    bsc = evbsc_new_w_defaults( loop, host, port, onerror, &errstr);
    fail_if( bsc == NULL, "evbsc_new: %s", errstr);

    BSC_ENQ_CMD(ignore,  bsc, NULL, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, NULL, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, NULL, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, fin_cb, exp_data, cmd_error, "default");

    ev_loop(loop, 0);
    fail_if(fail, "got invalid data");
    return;

cmd_error:
    evbsc_free(bsc);
    fail("cmd_error");
}
END_TEST

void reconnect_test_cb(evbsc *bsc, queue_node *node, void *data, size_t len)
{
    system(kill_cmd);
}

static void reconnect(evbsc *bsc, evbsc_error_t error)
{
    int i;
    char *errorstr;
    system(spawn_cmd);

    if (error == EVBSC_ERROR_INTERNAL) {
        fprintf(stderr, "critical error: recieved EVBSC_ERROR_INTERNAL, quitting\n");
        exit(1);
    }
    else if (error == EVBSC_ERROR_SOCKET) {
        BSC_ENQ_CMD(put,     bsc, NULL, NULL, cmd_error, 1, 0, 10, strlen(kill_cmd), kill_cmd);
        fprintf(stderr, "error: recieved EVBSC_ERROR_SOCKET, attempting to reconnect ...\n");
        if ( evbsc_reconnect(bsc, &errorstr) ) {
            fail_if( IOQ_NODES_USED(bsc->outq) != 4, 
                "after reconnect: IOQ_NODES_USED : %d/%d", IOQ_NODES_USED(bsc->outq), 4);

            ev_unloop(EV_A_ EVUNLOOP_ALL);
            return;
        }
    }
    fail("critical error: maxed out reconnect attempts, quitting\n");
    ev_unloop(EV_A_ EVUNLOOP_ALL);
    return;

cmd_error:
    evbsc_free(bsc);
    fail("cmd_error");
    ev_unloop(EV_A_ EVUNLOOP_ALL);
}

START_TEST(test_evbsc_reconnect) {
    loop = ev_default_loop(0);
    char *errstr = NULL;
    sprintf(spawn_cmd, "beanstalkd -p %s -d", reconnect_test_port);
    sprintf(kill_cmd, "ps -ef|grep beanstalkd |grep '%s'| gawk '!/grep/ {print $2}'|xargs kill", reconnect_test_port);
    system(spawn_cmd);
    bsc = evbsc_new( loop, host, reconnect_test_port, reconnect, 5, 12, 4, &errstr);
    fail_if( bsc == NULL, "evbsc_new: %s", errstr);

    BSC_ENQ_CMD(ignore,  bsc, reconnect_test_cb, NULL, cmd_error, "default");
    BSC_ENQ_CMD(reserve,  bsc, reconnect_test_cb, "1", cmd_error);
    BSC_ENQ_CMD(reserve,  bsc, reconnect_test_cb, "1", cmd_error);
    BSC_ENQ_CMD(reserve,  bsc, reconnect_test_cb, "1", cmd_error);

    ev_loop(loop, 0);
    system(kill_cmd);
    return;

cmd_error:
    evbsc_free(bsc);
    fail("cmd_error");
}
END_TEST

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("evbsc");

    tcase_add_test(tc, test_evbsc_small_vec);
    tcase_add_test(tc, test_evbsc_defaults);
    tcase_add_test(tc, test_evbsc_reconnect);

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
