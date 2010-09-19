/**
 * =====================================================================================
 * @file     ioqueue.c
 * @brief    A queue (buffer) library for event driven applications.
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

#include <stdlib.h>

#include "ioqueue.h"

#define IOQ_DUMP_FIN(q, n) do {                        \
    size_t __iter;                                     \
    for ( __iter = 0; __iter < (n); ++__iter ) {       \
        if ( AQ_REAR_(q)->autofree )                   \
            free(AQ_REAR_(q)->vec->iov_base);          \
        AQ_DEQ_FIN(q);                                 \
    }                                                  \
} while (0)

ioq *ioq_new(size_t size)
{
    register size_t i;
    struct iovec *vec = NULL;
    ioq *q = NULL;

    if ( ( q = (ioq *)malloc(sizeof(ioq)) ) == NULL )
        return NULL;

    if ( ( q->nodes = (ioq_node *)malloc( sizeof(ioq_node) * size ) ) == NULL )
        goto nodes_malloc_error;

    if ( ( vec = (struct iovec *)malloc(sizeof(struct iovec) * size) ) == NULL )
        goto vec_malloc_error;

    for (i = 0; i < size; ++i)
        q->nodes[i].vec = vec+i;

    q->size = size;
    q->rear = q->front = 0;
    q->used = 0;

    return q;

vec_malloc_error:
    free(q->nodes);
nodes_malloc_error:
    free(q);
    return NULL;
}

void ioq_enq_(ioq *q, void *data, ssize_t data_len, int autofree)
{
    AQ_FRONT_(q)->vec->iov_base = data;
    AQ_FRONT_(q)->vec->iov_len  = data_len;
    AQ_FRONT_(q)->autofree      = autofree;
    AQ_ENQ_FIN(q);
}

int ioq_enq(ioq *q, void *data, ssize_t data_len, int autofree)
{
    if (AQ_FULL(q)) return 0;

    ioq_enq_(q, data, data_len, autofree);
    return 1;
}

void ioq_free(ioq *q)
{
    IOQ_DUMP_FIN(q, q->used);
    free(q->nodes);
    free(q);
}

ssize_t ioq_dump(ioq *q, int fd)
{
    size_t  bytes_expected = 0, nodes_ready = IOQ_NODES_READY(q), i;
    ssize_t bytes_written, nodes_written  = 0;

    for (i = 0; i < nodes_ready; ++i)
        bytes_expected += IOQ_PEEK_POS(q,i)->iov_len;

    if ( ( bytes_written = writev(fd, IOQ_REAR_(q), nodes_ready) ) < bytes_expected )
        switch (bytes_written) {
            case -1:
                return -1;
            default:
                while ( ( bytes_written -= IOQ_REAR_(q)->iov_len ) > 0 ) {
                    IOQ_DUMP_FIN(q, 1);
                    ++nodes_written;
                }
                if ( bytes_written < 0 ) {
                    IOQ_REAR_(q)->iov_base += IOQ_REAR_(q)->iov_len + bytes_written;
                    IOQ_REAR_(q)->iov_len   = -bytes_written;
                }
                return nodes_written;
        }
    else {
        IOQ_DUMP_FIN(q, nodes_ready);
        return nodes_ready;
    }
}
