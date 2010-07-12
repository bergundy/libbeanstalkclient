/**
 * =====================================================================================
 * @file   check_evbuffer.c
 * @brief  test suite for evbuffer functions and macros
 * @date   06/13/2010 05:58:28 PM
 * @author Roey Berman, (royb@walla.net.il), Walla!
 * =====================================================================================
 */

#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evbuffer.h"


#define BUFS 40

evbuffer *buf;
evbuffer_node *node;
char msg[255];

void callme(evbuffer_node *node, char *data)
{
    printf("%s\n", data);
}

START_TEST(test_evbuffer_size) {
    fail_if( (buf = evbuffer_new(BUFS) ) == NULL, "out of memory");
    node = buf->readp;
    int i;
    for ( i = 1; i < BUFS; ++i) {
        node = node->next;
    }
    fail_unless(node->next == buf->readp, "invalid buffer size");
}
END_TEST

START_TEST(test_evbuffer) {
    fail_if( (buf = evbuffer_new(2) ) == NULL, "out of memory");
    fail_unless( evbuffer_put(buf, "baba", 4, callme, 0), "failed put");
    fail_unless( evbuffer_put(buf, "baba2", 5, callme, 0), "failed put");
    fail_if( evbuffer_put(buf, "baba3", 5, callme, 0), "put did not return false" );
    node = NULL;
    fail_if( ( node = evbuffer_get(buf) ) == NULL, "failed get");
    fail_unless( strncmp(node->data, "baba", node->length) == 0, "retrieved wrong data from buffer");
    evbuffer_read_fin(buf);
    node = NULL;
    fail_if( ( node = evbuffer_get(buf) ) == NULL, "failed get");
    fail_unless( strncmp(node->data, "baba2", node->length) == 0, "retrieved wrong data from buffer");
    evbuffer_read_fin(buf);
    fail_unless( ( node = evbuffer_get(buf) ) == NULL, "get did not return NULL");
    node = NULL;
    fail_if( ( node = evbuffer_getcb(buf) ) == NULL, "getcb failed");
    evbuffer_cb_fin(buf);
    evbuffer_cb_fin(buf);
    fail_unless( evbuffer_put(buf, "baba", 4, callme, 0), "failed put after empty");
    node = NULL;
    fail_if( ( node = evbuffer_get(buf) ) == NULL, "failed get");
    fail_unless( strncmp(node->data, "baba", node->length) == 0, "retrieved wrong data from buffer");
    evbuffer_read_fin(buf);
}
END_TEST

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("evbuffer");

    tcase_add_test(tc, test_evbuffer);
    tcase_add_test(tc, test_evbuffer_size);

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
