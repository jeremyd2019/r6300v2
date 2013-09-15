/*
 * Copyright (C) NEC Europe Ltd., 2003
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __RELAY6_SOCKET_H_DEFINED
#define __RELAY6_SOCKET_H_DEFINED

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "dhcp6r.h"

fd_set readfd;
int fdmax;  

struct receive {
	struct msghdr msg;
	struct iovec iov[1];
	struct cmsghdr *cmsgp;
	struct sockaddr_in6 sin6;    /* my address information */
	struct sockaddr_in6 from;
	int recvmsglen;
	char *recvp;
	char src_addr[INET6_ADDRSTRLEN];
	int pkt_interface;
	int buflength;
	int dst_addr_type ;
	char *databuf;
	int recv_sock_desc;
};

struct send {
	int send_sock_desc;
};

struct send  *sendsock;
struct receive *recvsock;

int send_message __P((void));
int fill_addr_struct __P((void));
int set_sock_opt __P((void));
int recv_data __P((void));
int check_select __P((void));
int get_recv_data __P((void));
int get_interface_info __P((void));
void init_socket __P((void));

#endif /* __RELAY6_SOCKET_H_DEFINED */
