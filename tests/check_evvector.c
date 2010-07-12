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
#include "evvector.h"

evvector *vec;

START_TEST(test_evvector) {
    fail_if( (vec = evvector_new(2, 2) ) == NULL, "out of memory");
    vec->data[0] = 'a';
    vec->data[1] = '\0';
    fail_if( strcmp(vec->data, "a") != 0, "got bad data");
    vec->eom = vec->data + 1;
    fail_if( EVVECTOR_FREE(vec), "EVVECTOR_FREE");
    evvector_expand(vec);
    fail_if( EVVECTOR_FREE(vec) != 2, "EVVECTOR_EXPAND");
    *vec->eom++ = 'b';
    *vec->eom++ = 'c';
    *vec->eom   = '\0';
    fail_if( EVVECTOR_FREE(vec), "EVVECTOR_FREE");
    fail_if( strcmp(vec->som, "abc") != 0, "got bad data");
    evvector_free(vec);
}
END_TEST

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("evvector");

    tcase_add_test(tc, test_evvector);

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
