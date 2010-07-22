/**
 * =====================================================================================
 * @file   check_ivector.c
 * @brief  test suite for ivector functions
 * @date   06/13/2010 05:58:28 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ivector.h"

ivector *vec;

START_TEST(test_ivector) {
    fail_if( (vec = ivector_new(2) ) == NULL, "out of memory");
    vec->data[0] = 'a';
    vec->data[1] = '\0';
    fail_if( strcmp(vec->data, "a") != 0, "got bad data");
    vec->eom = vec->data + 1;
    fail_if( IVECTOR_FREE(vec), "IVECTOR_FREE");
    ivector_expand(vec);
    fail_if( IVECTOR_FREE(vec) != 2, "IVECTOR_EXPAND");
    *vec->eom++ = 'b';
    *vec->eom++ = 'c';
    *vec->eom   = '\0';
    fail_if( IVECTOR_FREE(vec), "IVECTOR_FREE");
    fail_if( strcmp(vec->som, "abc") != 0, "got bad data");
    ivector_free(vec);
}
END_TEST

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("ivector");

    tcase_add_test(tc, test_ivector);

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
