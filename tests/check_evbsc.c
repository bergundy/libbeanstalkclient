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
struct ev_loop *loop;
int counter = 0;

void small_vec_cb(evbuffer_node *node, char *data)
{
    printf("got data: '%s'\n", data);
    if (counter  > 8) {
        node->bytes_expected = 10;
    }
    if (++counter == 7) {
        ev_io_stop(EV_A_ &(bsc->ww));
        ev_unloop(EV_A_ EVUNLOOP_ALL);
    }
}

START_TEST(test_evbsc_small_vec) {
    loop = ev_default_loop(0);
    fail_if( (bsc = evbsc_new( loop, host, port, 5, 8, 12, 8, 4, NULL) ) == NULL, "out of memory");

    BSC_ENQ_CMD(use,    bsc, small_vec_cb, cmd_error, "test");
    BSC_ENQ_CMD(watch,  bsc, small_vec_cb, cmd_error, "test");
    BSC_ENQ_CMD(ignore, bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore, bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore, bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore, bsc, small_vec_cb, cmd_error, "default");
    BSC_ENQ_CMD(ignore, bsc, small_vec_cb, cmd_error, "default");
    ev_io_start(loop, &(bsc->ww));

    ev_loop(loop, 0);
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
