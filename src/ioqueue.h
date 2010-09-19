/**
 * =====================================================================================
 * @file     ioqueue.h
 * @brief    header file for ioqueue.c -
 *           A queue (buffer) library for event driven applications.
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

#ifndef _IOQUEUE_H
#define _IOQUEUE_H

#include <sys/uio.h>
#include <arrayqueue.h>

struct _ioq_node {
    struct iovec *vec;
    int    autofree;
};

AQ_DEFINE_STRUCT(_ioq, struct _ioq_node);

typedef struct _ioq ioq;
typedef struct _ioq_node ioq_node;

#define IOQ_NODES_READY(q)   ((q)->used ? ( (q)->front <= (q)->rear ? (q)->size - (q)->rear : (q)->used ) : 0)
#define IOQ_PEEK_POS(q, i)   ((AQ_REAR_(q)+i)->vec)
#define IOQ_REAR_(q)         IOQ_PEEK_POS(q,0)
#define IOQ_REAR(q)          (AQ_EMPTY(q) ? NULL : IOQ_REAR_(q))

void    ioq_enq_(ioq *q, void *data, ssize_t data_len, int autofree);
int     ioq_enq(ioq *q, void *data, ssize_t data_len, int autofree);
ssize_t ioq_dump(ioq *q, int fd);
ioq    *ioq_new(size_t size);
void    ioq_free(ioq *q);

#endif /* _IOQUEUE_H */
