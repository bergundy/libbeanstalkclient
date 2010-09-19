/*
 * =====================================================================================
 *
 *       Filename:  test_commands.c
 *
 *    Description:  test suite for libbeanstalkproto command generation funcions
 *
 *        Version:  1.0
 *        Created:  05/22/2010 09:29:13 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Roey Berman, roey.berman@gmail.com
 *
 * =====================================================================================
 */

#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "beanstalkproto.h"

#define TEST_MSG(func_name, expected, ...)                                                                                     \
void test_ ##func_name(int _i CK_ATTRIBUTE_UNUSED)                                                                             \
{                                                                                                                              \
    tcase_fn_start("test_" #func_name, __FILE__, __LINE__);                                                                    \
    {                                                                                                                          \
        char *msg, *expected_msg = expected;                                                                                   \
        bool is_allocated;                                                                                                     \
        int  msg_len, expected_msg_len = strlen(expected_msg);                                                                 \
        msg = func_name( &msg_len, &is_allocated, ##__VA_ARGS__ );                                                             \
        fail_unless( strcmp(msg, expected_msg) == 0, #func_name  " -> got: '%s', expected: '%s'", msg, expected_msg );         \
        fail_unless( msg_len == expected_msg_len, #func_name "(length) -> got %d, expected %d", msg_len, expected_msg_len );   \
        if (is_allocated) free(msg);                                                                                           \
    }                                                                                                                          \
}                                                                                                                              \
tcase_add_test(tc, test_ ##func_name);

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create(__FILE__);

    TEST_MSG( bsp_gen_put_hdr,                "put 1 2 3 4\r\n",            1, 2, 3, 4);
    TEST_MSG( bsp_gen_use_cmd,                "use baba\r\n",               "baba" );
    TEST_MSG( bsp_gen_reserve_cmd,            "reserve\r\n" );
    TEST_MSG( bsp_gen_reserve_with_to_cmd,    "reserve-with-timeout 1\r\n", 1 );
    TEST_MSG( bsp_gen_delete_cmd,             "delete 12345\r\n",           12345 );
    TEST_MSG( bsp_gen_release_cmd,            "release 12345 3 4\r\n",      12345, 3, 4 );
    TEST_MSG( bsp_gen_bury_cmd,               "bury 12345 3\r\n",           12345, 3 );
    TEST_MSG( bsp_gen_touch_cmd,              "touch 12345\r\n",            12345 );
    TEST_MSG( bsp_gen_watch_cmd,              "watch baba\r\n",             "baba" );
    TEST_MSG( bsp_gen_ignore_cmd,             "ignore baba\r\n",            "baba" );
    TEST_MSG( bsp_gen_peek_cmd,               "peek 4\r\n",                 4 );
    TEST_MSG( bsp_gen_peek_ready_cmd,         "peek-ready\r\n"   );
    TEST_MSG( bsp_gen_peek_delayed_cmd,       "peek-delayed\r\n" );
    TEST_MSG( bsp_gen_peek_buried_cmd,        "peek-buried\r\n"  );
    TEST_MSG( bsp_gen_kick_cmd,               "kick 4\r\n",                 4 );
    TEST_MSG( bsp_gen_stats_job_cmd,          "stats-job 4\r\n",            4 );
    TEST_MSG( bsp_gen_stats_tube_cmd,         "stats-tube baba\r\n",        "baba" );
    TEST_MSG( bsp_gen_stats_cmd,              "stats\r\n" );
    TEST_MSG( bsp_gen_quit_cmd,               "quit\r\n"  );
    TEST_MSG( bsp_gen_list_tubes_cmd,         "list-tubes\r\n" );
    TEST_MSG( bsp_gen_list_tubes_watched_cmd, "list-tubes-watched\r\n" );
    TEST_MSG( bsp_gen_pause_tube_cmd,         "pause-tube baba 2345\r\n", "baba", 2345 );

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
