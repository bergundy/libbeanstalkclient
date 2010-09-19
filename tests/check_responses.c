/*
 * =====================================================================================
 *
 *       Filename:  test_responses.c
 *
 *    Description:  test suite for libbeanstalkproto response parsing funcions
 *
 *        Version:  1.0
 *        Created:  06/01/2010 06:33:23 PM
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

static int intcmp( int a, int b )
{
    return a - b;
}

#define TEST_RES(func_name, test_input, exp_t)                                                            \
void test_ ## func_name ## _  ## exp_t(int _i CK_ATTRIBUTE_UNUSED)                                        \
{                                                                                                         \
    tcase_fn_start("test_" #func_name #exp_t, __FILE__, __LINE__);                                        \
    {                                                                                                     \
        bsc_response_t got_t;                                                                             \
        got_t = func_name( test_input );                                                                  \
        fail_unless( exp_t == got_t, #func_name "(return code) -> got %d, expected %d", got_t, exp_t );   \
    }                                                                                                     \
}                                                                                                         \
tcase_add_test(tc, test_ ## func_name ## _ ## exp_t);

#define TEST_RES1(func_name, test_input, exp_t, arg_type, exp_arg_res, compare_func, validate_arg)        \
void test_ ## func_name ## _  ## exp_t(int _i CK_ATTRIBUTE_UNUSED)                                        \
{                                                                                                         \
    tcase_fn_start("test_" #func_name #exp_t, __FILE__, __LINE__);                                        \
    {                                                                                                     \
        arg_type arg;                                                                                     \
        bsc_response_t got_t;                                                                             \
        got_t = func_name( test_input, &arg );                                                            \
        fail_unless( exp_t == got_t, #func_name "(return code) -> got %d, expected %d", got_t, exp_t );   \
        if (validate_arg)                                                                                 \
            if ( strcmp(#arg_type, "uint32_t" ) == 0 )                                                    \
            fail_unless( compare_func(arg, exp_arg_res) == 0,                                             \
                #func_name  "(arg) -> got: '%u', expected: '%u'", arg, exp_arg_res );                     \
            else if ( strcmp(#arg_type, "char *" ) == 0 )                                                 \
            fail_unless( compare_func(arg, exp_arg_res) == 0,                                             \
                #func_name  "(arg) -> got: '%s', expected: '%s'", arg, exp_arg_res );                     \
    }                                                                                                     \
}                                                                                                         \
tcase_add_test(tc, test_ ## func_name ## _ ## exp_t);

#define TEST_RES3(func_name, test_input, exp_t, exp_id, validate_arg )                                    \
void test_ ## func_name ## _  ## exp_t(int _i CK_ATTRIBUTE_UNUSED)                                        \
{                                                                                                         \
    tcase_fn_start("test_" #func_name #exp_t, __FILE__, __LINE__);                                        \
    {                                                                                                     \
        uint64_t id;                                                                                      \
        uint32_t bytes;                                                                                   \
        bsc_response_t got_t;                                                                             \
        char *input_dup = strdup(test_input);                                                             \
        got_t = func_name( input_dup, &id, &bytes );                                                      \
        fail_unless( exp_t == got_t, #func_name "(return code) -> got %d, expected %d", got_t, exp_t );   \
        if (validate_arg) {                                                                               \
            fail_unless( id == exp_id,                                                                    \
                #func_name  "(id) -> got: '%u', expected: '%u'", id, exp_id );                            \
            fail_unless( bytes > 0, #func_name  "(bytes)" );                                              \
        }                                                                                                 \
    }                                                                                                     \
}                                                                                                         \
tcase_add_test(tc, test_ ## func_name ## _ ## exp_t);

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create(__FILE__);

    /* put responses */
    TEST_RES1(bsp_get_put_res, "INSERTED 4\r\n",      BSC_PUT_RES_INSERTED,      uint64_t, 4, intcmp, 1);
    TEST_RES1(bsp_get_put_res, "BURIED 4\r\n",        BSC_RES_BURIED,            uint64_t, 4, intcmp, 1);
    TEST_RES1(bsp_get_put_res, "EXPECTED_CRLF\r\n",   BSC_PUT_RES_EXPECTED_CRLF, uint64_t, 0, intcmp, 0);
    TEST_RES1(bsp_get_put_res, "JOB_TOO_BIG\r\n",     BSC_PUT_RES_JOB_TOO_BIG,   uint64_t, 0, intcmp, 0);
    TEST_RES1(bsp_get_put_res, "OUT_OF_MEMORY\r\n",   BSC_RES_OUT_OF_MEMORY,     uint64_t, 0, intcmp, 0);
    TEST_RES1(bsp_get_put_res, "INTERNAL_ERROR\r\n",  BSC_RES_INTERNAL_ERROR,    uint64_t, 0, intcmp, 0);
    TEST_RES1(bsp_get_put_res, "UNKNOWN_COMMAND\r\n", BSC_RES_UNKNOWN_COMMAND,   uint64_t, 0, intcmp, 0);

    /* use responses */
    TEST_RES1(bsp_get_use_res, "USING bar\r\n",       BSC_USE_RES_USING,         char *,   "bar", strcmp, 1);
    TEST_RES1(bsp_get_use_res, "OUT_OF_MEMORY\r\n",   BSC_RES_OUT_OF_MEMORY,     char *,   "bar", strcmp, 0);
    
    /* reserve responses */
    TEST_RES3( bsp_get_reserve_res, "RESERVED 3456543 3\r\n", BSC_RESERVE_RES_RESERVED,      3456543, 1);
    TEST_RES3( bsp_get_reserve_res, "DEADLINE_SOON\r\n",      BSC_RESERVE_RES_DEADLINE_SOON, 3456543, 0);
    TEST_RES3( bsp_get_reserve_res, "TIMED_OUT\r\n",          BSC_RESERVE_RES_TIMED_OUT,     3456543, 0);
    TEST_RES3( bsp_get_reserve_res, "INTERNAL_ERROR\r\n",     BSC_RES_INTERNAL_ERROR,        3456543, 0);
    TEST_RES3( bsp_get_reserve_res, "UNKNOWN_COMMAND\r\n",    BSC_RES_UNKNOWN_COMMAND,       3456543, 0);

    /* delete responses */
    TEST_RES(  bsp_get_delete_res,   "DELETED\r\n",        BSC_DELETE_RES_DELETED   );
    TEST_RES(  bsp_get_delete_res,   "NOT_FOUND\r\n",      BSC_RES_NOT_FOUND        );
    TEST_RES(  bsp_get_delete_res,   "OUT_OF_MEMORY\r\n",  BSC_RES_OUT_OF_MEMORY    );

    /* release responses */
    TEST_RES(  bsp_get_release_res,  "RELEASED\r\n",       BSC_RELEASE_RES_RELEASED );
    TEST_RES(  bsp_get_release_res,  "BURIED\r\n",         BSC_RES_BURIED           );
    TEST_RES(  bsp_get_release_res,  "NOT_FOUND\r\n",      BSC_RES_NOT_FOUND        );
    TEST_RES(  bsp_get_release_res,  "INTERNAL_ERROR\r\n", BSC_RES_INTERNAL_ERROR   );

    /* bury responses */
    TEST_RES(  bsp_get_bury_res,     "BURIED\r\n",         BSC_RES_BURIED           );
    TEST_RES(  bsp_get_bury_res,     "NOT_FOUND\r\n",      BSC_RES_NOT_FOUND        );
    TEST_RES(  bsp_get_bury_res,     "INTERNAL_ERROR\r\n", BSC_RES_INTERNAL_ERROR   );

    /* touch responses */
    TEST_RES(  bsp_get_touch_res,    "TOUCHED\r\n",         BSC_TOUCH_RES_TOUCHED    );
    TEST_RES(  bsp_get_touch_res,    "NOT_FOUND\r\n",       BSC_RES_NOT_FOUND        );
    TEST_RES(  bsp_get_touch_res,    "UNKNOWN_COMMAND\r\n", BSC_RES_UNKNOWN_COMMAND  );

    /* watch responses */
    TEST_RES1(bsp_get_watch_res, "WATCHING 4\r\n",      BSC_RES_WATCHING,       uint32_t, 4, intcmp, 1);
    TEST_RES1(bsp_get_watch_res, "OUT_OF_MEMORY\r\n",   BSC_RES_OUT_OF_MEMORY,  uint32_t, 0, intcmp, 0);

    /* ignore responses */
    TEST_RES1(bsp_get_ignore_res, "WATCHING 4\r\n",      BSC_RES_WATCHING,           uint32_t, 4, intcmp, 1);
    TEST_RES1(bsp_get_ignore_res, "NOT_IGNORED\r\n",     BSC_IGNORE_RES_NOT_IGNORED, uint32_t, 0, intcmp, 0);
    TEST_RES1(bsp_get_ignore_res, "OUT_OF_MEMORY\r\n",   BSC_RES_OUT_OF_MEMORY,      uint32_t, 0, intcmp, 0);

    /* peek responses */
    TEST_RES3( bsp_get_peek_res, "FOUND 3456543 3\r\n", BSC_PEEK_RES_FOUND,        3456543, 1);
    TEST_RES3( bsp_get_peek_res, "NOT_FOUND\r\n",       BSC_RES_NOT_FOUND,         3456543, 0);
    TEST_RES3( bsp_get_peek_res, "INTERNAL_ERROR\r\n",  BSC_RES_INTERNAL_ERROR,    3456543, 0);
    TEST_RES3( bsp_get_peek_res, "GIBRISH\r\n",         BSC_RES_UNRECOGNIZED, 3456543, 0);

    /* kick responses */
    TEST_RES1(bsp_get_kick_res, "KICKED 4\r\n",        BSC_KICK_RES_KICKED,        uint32_t, 4, intcmp, 1);
    TEST_RES1(bsp_get_kick_res, "OUT_OF_MEMORY\r\n",   BSC_RES_OUT_OF_MEMORY,      uint32_t, 0, intcmp, 0);

    /* pause-tube responses */
    TEST_RES(  bsp_get_pause_tube_res,    "PAUSED\r\n",          BSC_PAUSE_TUBE_RES_PAUSED  );
    TEST_RES(  bsp_get_pause_tube_res,    "NOT_FOUND\r\n",       BSC_RES_NOT_FOUND          );
    TEST_RES(  bsp_get_pause_tube_res,    "GIBRISH\r\n",         BSC_RES_UNRECOGNIZED  );
    suite_add_tcase(s, tc);

    return s;
}

int main() {
    SRunner *sr;
    Suite *s;
    int failed;

    s = local_suite();
    sr = srunner_create(s);
    //srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);

    failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
