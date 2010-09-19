/**
 * =====================================================================================
 * @file     sockutils.c
 * @brief    helper functions for sockets.
 * @date     08/09/2010
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
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "sockutils.h"

/* ================================================================================
 * error handling
 * ================================================================================ */
#ifdef _GNU_SOURCE
void STRERROR(int errnum, char *errorstr)
{
    char *p = NULL;

    if ((p = strerror_r(errnum, errorstr, SOCK_ERRSTR_LEN)) == NULL)
        sprintf(errorstr, "Error string too short to hold errno: %d", errnum);
        
    if (p != errstr)
        strcpy(errorstr, p);
}
#else
void STRERROR(int errnum, char *errorstr)
{
    if (strerror_r(errnum, errorstr, SOCK_ERRSTR_LEN) < 0) {
        if (errno == ERANGE)
            sprintf(errorstr, "Error string too short to hold errno: %d", errnum);
        else
            sprintf(errorstr, "Unkown error: %d", errnum);
    }
}
#endif

#define CONST_STRLEN(s) (sizeof(s)/sizeof(char)-1)

#define sperror(where) do {                                     \
    if (errorstr != NULL) {                                     \
        memcpy(errorstr, where ": ", CONST_STRLEN(where ": ")); \
        STRERROR(errno, errorstr+CONST_STRLEN(where ": "));     \
    }                                                           \
} while (0)

/* ================================================================================
 * socket flags (fcntl wrapper functions)
 * ================================================================================ */
int unblock(int sock, char *errorstr)
{
    return set_sock_flags(sock, O_NONBLOCK, errorstr);
}

int set_sock_flags(int sock, int set_flags, char *errorstr)
{
    int flags;

    if (( flags = fcntl(sock, F_GETFL, NULL) ) < 0) { 
        sperror("F_GETFL");
        return 0;
    } 
    if (fcntl(sock, F_SETFL, flags | set_flags) < 0) { 
        sperror("F_SETFL");
        return 0;
    } 
    return 1;
}

int unset_sock_flags(int sock, int unset_flags, char *errorstr)
{
    int flags;

    if (( flags = fcntl(sock, F_GETFL, NULL) ) < 0) { 
        sperror("F_GETFL");
        return 0;
    } 
    if (fcntl(sock, F_SETFL, flags & ~unset_flags) < 0) { 
        sperror("F_SETFL");
        return 0;
    } 
    return 1;
}

/* ================================================================================
 * addrinfo
 * ================================================================================ */
struct addrinfo *prepare_addrinfo_tcp(const char *addr,
                                      const char *port,
                                      char *errorstr)
{
    struct addrinfo hints, *res;
    int gai;

    /*  init addrinfo */
    memset(&hints, 0, sizeof hints);
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags    = AI_PASSIVE;     // fill in my IP for me

    if (( gai = getaddrinfo(addr, port, &hints, &res)) != 0) {
        if (errorstr != NULL)
            sprintf(errorstr, "gai: %s", gai_strerror(gai));
        return NULL;
    }

    return res;
}

/* ================================================================================
 * tcp server / client
 * ================================================================================ */
int tcp_server(const char *bind_addr, const char *port, size_t backlog, char *errorstr)
{
    struct addrinfo *ai;
    int              sockfd, retsockfd = -1;

    if (( ai = prepare_addrinfo_tcp(bind_addr, port, errorstr) ) == NULL)
        return SOCK_ERR;

    /* socket creation */
    if ((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
        sperror("socket");
        goto done;
    }

    /* bind */
    if (bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        sperror("bind");
        close(sockfd);
        goto done;
    }

    /* listen */
    if (listen(sockfd, backlog) < 0) {
        sperror("listen");
        close(sockfd);
        goto done;
    }

    retsockfd = sockfd;

done:
    freeaddrinfo(ai);
    return retsockfd;
}

int tcp_client(char const *server_addr, char const *port, char *errorstr)
{
    struct addrinfo *servinfo, *p;
    int              sockfd;
    
    if ( (servinfo = prepare_addrinfo_tcp(server_addr, port, errorstr)) == NULL)
        return SOCK_ERR;

    /* loop through all the results and connect to the first we can */
    for (p = servinfo; p != NULL; p = p->ai_next) {

        /* socket creation */
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) < 0) {
            sperror("socket");
            continue;
        }

        /* connect */
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
            close(sockfd);
            sperror("connect");
            continue;
        }

        break;
    }

    if (p == NULL)
        sockfd = SOCK_ERR;

    freeaddrinfo(servinfo); // all done with this structure
    return sockfd;
}
