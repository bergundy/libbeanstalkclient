/**
 * =====================================================================================
 * @file     sockutils.h
 * @brief    header file for sockutils.c - helper functions for sockets.
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

#ifndef SOCKUTILS_H
#define SOCKUTILS_H 

#include <sys/types.h>

#define SOCK_ERR         -1
#define SOCK_ERRSTR_LEN 512

/** 
* sets sock to nonblocking mode.
* 
* @param sock      the file descriptor to unblock
* @param errorstr  a string to store the error in
* 
* @return          1 on success 0 on failure
*/
int unblock(int sock, char *errorstr);

/** 
* an fcntl wrapper.
* 
* @param sock      the file descriptor to set flags on
* @param set_flags flags to set (binary OR)
* @param errorstr  a string to store the error in
* 
* @return          1 on success 0 on failure
*/
int set_sock_flags(int sock, int set_flags, char *errorstr);

/** 
* an fcntl wrapper.
* 
* @param sock          the file descriptor to unset flags on
* @param unset_flags   flags to unset (binary AND NOT)
* @param errorstr      a string to store the error in
* 
* @return              1 on success 0 on failure
*/
int unset_sock_flags(int sock, int unset_flags, char *errorstr);

/** 
* creates a tcp socket listening on bind_addr:port
* 
* @param bind_addr  the address to bind to
* @param port       the port to bind to
* @param backlog    see listen (3)
* @param errorstr   a string to store the error in
* 
* @return           a file descriptor or SOCKERR on error
*/
int tcp_server(const char *bind_addr, const char *port, size_t backlog, char *errorstr);

/** 
* creates a tcp client connected to server on server_addr:port
* 
* @param server_addr  the server's address
* @param port         the server's port
* @param errorstr     a string to store the error in
* 
* @return             a file descriptor or SOCKERR on error
*/
int tcp_client(char const *server_addr, char const *port, char *errorstr);

#endif /* SOCKUTILS_H */
