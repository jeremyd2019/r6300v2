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

#ifndef __RELAY6_PARSER_H_DEFINED
#define __RELAY6_PARSER_H_DEFINED

#include "dhcp6r.h"
#include "relay6_socket.h"

struct msg_parser {
	struct msg_parser *next;
	struct msg_parser *prev;
    
	int if_index;
	uint8_t msg_type;
	uint8_t hop;
	uint8_t *buffer;
	uint8_t *ptomsg;
	uint8_t *pstart, *pointer_start, *hc_pointer;
	uint32_t datalength;  /* the length of the DHCPv6 message */
	int dst_addr_type;
	char src_addr[INET6_ADDRSTRLEN];  /* source address from the UDP packet */
	char peer_addr[INET6_ADDRSTRLEN];
	char link_addr[INET6_ADDRSTRLEN];
	int interface_in, hop_count;
	int sent;
	int isRF;
};

struct msg_parser msg_parser_list;
 
struct msg_parser *create_parser_obj __P((void));
int put_msg_in_store __P((struct msg_parser *mesg));
int check_buffer __P((int ref, struct msg_parser *mesg));

#endif /* __RELAY6_PARSER_H_DEFINED */
