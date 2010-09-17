/**
 * =====================================================================================
 * @file   cbq.c
 * @brief  
 * @date   08/15/2010 08:44:03 PM
 * @author Roey Berman, (roey.berman@gmail.com)
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
#include "cbq.h"

cbq *cbq_new(size_t size)
{
    cbq *q;
    union bsc_cmd_info *cmd_info;
    register size_t i;

    if ( ( q = (cbq *)malloc(sizeof(cbq)) ) == NULL )
        return NULL;

    if ( ( q->nodes = (cbq_node *)malloc(sizeof(cbq_node) * size) ) == NULL )
        goto node_malloc_error;

    if ( ( cmd_info = (union bsc_cmd_info *)malloc(sizeof(union bsc_cmd_info) * size) ) == NULL )
        goto cmd_info_malloc_error;

    for (i = 0; i < size; ++i)
        q->nodes[i].cb_data = cmd_info+i;

    q->size = size;
    q->rear = q->front = 0;
    q->used = 0;

    return q;

cmd_info_malloc_error:
    free(q->nodes);
node_malloc_error:
    free(q);
    return NULL;
}

void cbq_free(cbq *q)
{
    while ( !AQ_EMPTY(q) )
        CBQ_DEQ_FIN(q);
    free(q->nodes[0].cb_data);
    free(q);
}
