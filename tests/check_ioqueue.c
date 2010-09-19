/**
 * =====================================================================================
 * @file     check_ioqueue.c
 * @brief    test suite for libioqueue functions and macros.
 * @date     07/05/2010
 * @author   Roey Berman, (roey.berman@gmail.com)
 * @version  1.0
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "ioqueue.h"

#define Q_SIZE 4

START_TEST(test_ioqueue)
{
    ioq *q = ioq_new(Q_SIZE);
    struct iovec *iov;
    char msg[200];
    int i, bytes_expected = 0;

    /* ioq_enq */
    for (i=0;i<Q_SIZE;++i) {
        sprintf(msg, "msg[%d]", i);
        fail_unless(ioq_enq(q, strdup(msg), strlen(msg), 1), "ioq_enq");
        printf("put: [%d] %s (%d)\n", i, IOQ_PEEK_POS(q,i)->iov_base, IOQ_PEEK_POS(q,i)->iov_len);
    }
    fail_if(ioq_enq(q, "baba4", sizeof("baba4") - 1, 0) != 0, "ioq_enq on full queue");

    for (i=0;i<Q_SIZE;++i) {
        sprintf(msg, "msg[%d]", i);
        printf("get: [%d] %s (%d)\n", (int)q->rear, IOQ_REAR_(q)->iov_base, IOQ_REAR_(q)->iov_len);
        fail_unless(iov = IOQ_REAR(q), "IOQ_REAR");
        fail_if(strcmp((iov->iov_base), msg), "node->iov_base != msg (%s/%s)", iov->iov_base, msg);
        AQ_DEQ_FIN(q);
    }

    for (i=0;i<Q_SIZE;++i) {
        sprintf(msg, "msg[%d]", i);
        fail_unless(ioq_enq(q, strdup(msg), strlen(msg), 1), "ioq_enq");
        printf("put: [%d] %s (%d)\n", i, IOQ_PEEK_POS(q,i)->iov_base, IOQ_PEEK_POS(q,i)->iov_len);
    }
    AQ_DEQ_FIN(q);
    fail_unless(IOQ_NODES_READY(q) == 3, "IOQ_NODES_READY : %d/%d", IOQ_NODES_READY(q), 3);
    fail_unless(AQ_NODES_FREE(q) == 1, "AQ_NODES_FREE : %d/%d", AQ_NODES_FREE(q), 1);
    sprintf(msg, "msg[0]");
    fail_unless(ioq_enq(q, strdup(msg), strlen(msg), 1), "ioq_enq");
    fail_unless(AQ_NODES_FREE(q) == 0, "AQ_NODES_FREE : %d/%d", AQ_NODES_FREE(q), 0);
    fail_unless(q->used == 4, "IOQ_NODES_USED : %d/%d", q->used, 4);

    for (i = 0; i < IOQ_NODES_READY(q); ++i)
        bytes_expected += IOQ_PEEK_POS(q,i)->iov_len;

    fail_unless(bytes_expected == strlen(msg)*3, "IOQ_BYTES_EXPECTED : %d/%d", bytes_expected, strlen(msg)*3);
    i = ioq_dump(q, 1);
    fail_if(i != 3, "ioq_write: %d/%d", i, 3);
    fail_unless(IOQ_NODES_READY(q) == q->used, "IOQ_NODES_READY : %d/%d", IOQ_NODES_READY(q), q->used);
    fail_if(ioq_dump(q, 1) != 1, "ioq_write (2)");
    fail_unless(AQ_EMPTY(q), "IOQ_EMPTY : %d/%d", AQ_EMPTY(q), 0);
}
END_TEST

Suite *local_suite(void)
{
    Suite *s  = suite_create(__FILE__);
    TCase *tc = tcase_create("ioqueue");

    tcase_add_test(tc, test_ioqueue);

    suite_add_tcase(s, tc);
    return s;
}

int main()
{
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
