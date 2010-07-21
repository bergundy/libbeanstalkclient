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
#include <sys/select.h>
#include "evbsc.h"

evbsc *bsc;
char *host = "localhost", *port = BSC_DEFAULT_PORT;
struct ev_loop *loop;
int counter = 0;
int fail = 0, finished = 0;
char *exp_data;

void small_vec_cb(evbsc *bsc, queue_node *node, void *data, size_t len)
{
    printf("got data: '%s'\n", data);
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
            BSC_ENQ_CMD(delete, bsc, small_vec_cb, cmd_error, res_id );
            break;
        case 10:
        case 14:
            res = bsc_get_delete_res(data);
            if ( res != BSC_DELETE_RES_DELETED )
                ++fail;
            else if (counter == 10) {
                exp_data = "bababuba12341234";
                BSC_ENQ_CMD(put,     bsc, small_vec_cb, cmd_error, 1, 0, 10, strlen(exp_data), exp_data);
                BSC_ENQ_CMD(reserve, bsc, small_vec_cb, cmd_error );
                break;
            }
        default:
            ++finished;
    }
    ++counter;
    if ( len != strlen(data) )
        fail = 1;

    return;

cmd_error:
    ++fail;
}

void fin_cb(evbsc *bsc, queue_node *node, void *data, size_t len)
{
}

void onerror(evbsc *bsc, evbsc_error_t error)
{
    ++fail;
    fail("evbsc got error");
}

START_TEST(test_evbsc_small_vec) {
    char *errstr = NULL;
    bsc = evbsc_new(host, port, onerror, 11, 12, 4, &errstr);
    fail_if( bsc == NULL, "evbsc_new: %s", errstr);
    exp_data = "baba";
    fd_set readset, writeset;
    int i;

    BSC_ENQ_CMD(use,     bsc, small_vec_cb, cmd_error, "test");
    BSC_ENQ_CMD(watch,   bsc, small_vec_cb, cmd_error, "test");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, NULL, cmd_error, "default");
    BSC_ENQ_CMD(put,     bsc, small_vec_cb, cmd_error, 1, 0, 10, strlen(exp_data), exp_data);
    BSC_ENQ_CMD(reserve, bsc, small_vec_cb, cmd_error );

    FD_ZERO(&readset);
    FD_ZERO(&writeset);

    while (!fail && !finished) {
        FD_SET(bsc->fd, &readset);
        FD_SET(bsc->fd, &writeset);
        if (IOQ_EMPTY(bsc->outq)) {
            if ( select(bsc->fd+1, &readset, NULL, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(bsc->fd, &readset)) {
                printf("reading..\n");
                evbsc_read(bsc);
            }
        }
        else {
            if ( select(bsc->fd+1, &readset, &writeset, NULL, NULL) < 0) {
                fail("select()");
                return;
            }
            if (FD_ISSET(bsc->fd, &readset)) {
                printf("reading..\n");
                evbsc_read(bsc);
            }
            if (FD_ISSET(bsc->fd, &writeset)) {
                printf("writing..\n");
                evbsc_write(bsc);
            }
        }
    }

    fail_if(fail, "got invalid data");

    return;

cmd_error:
    evbsc_free(bsc);
    fail("cmd_error");
}
END_TEST

#if 0
START_TEST(test_evbsc_defaults) {
    char *errstr = NULL;
    fd_set readset, writeset;
    int i;
    bsc = evbsc_new_w_defaults(host, port, onerror, &errstr);
    fail_if( bsc == NULL, "evbsc_new: %s", errstr);

    BSC_ENQ_CMD(ignore,  bsc, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, NULL, cmd_error, "default");
    BSC_ENQ_CMD(ignore,  bsc, fin_cb, cmd_error, "default");

    fail_if(fail, "got invalid data");

    FD_ZERO(&readset);
    FD_ZERO(&writeset);
    FD_SET(bsc->fd, &readset);
    FD_SET(bsc->fd, &writeset);

    while (!fail && !finished) {
        if ( select(bsc->fd+1, &readset, &writeset, NULL, NULL) < 0) {
            fail("select()");
            return;
        }
        if (FD_ISSET(bsc->fd, &readset)) {
            printf("reading..\n");
            evbsc_read(bsc);
        }
        if (FD_ISSET(bsc->fd, &writeset)) {
            if (!IOQ_EMPTY(bsc->outq)) {
                printf("writing..\n");
                evbsc_write(bsc);
            }
        }
    }

    return;

cmd_error:
    evbsc_free(bsc);
    fail("cmd_error");
}
END_TEST
#endif


Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("evbsc");

    tcase_add_test(tc, test_evbsc_small_vec);
    //tcase_add_test(tc, test_evbsc_defaults);

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
