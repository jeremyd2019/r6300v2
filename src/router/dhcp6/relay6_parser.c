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

#include <stdlib.h>
#include <string.h>

#include "relay6_parser.h"
#include "relay6_database.h"

struct msg_parser *create_parser_obj() 
{
	struct msg_parser *msg;
  
	msg = (struct msg_parser *) malloc (sizeof(struct msg_parser));
	if (msg == NULL) {
		printf("create_parser_obj()--> NO MORE MEMORY AVAILABLE\n"); 
		exit(1);
	}  
  
	msg->buffer = (uint8_t *) malloc(MAX_DHCP_MSG_LENGTH*sizeof(uint8_t));
	if (msg->buffer == NULL) {
		printf("create_parser_obj()--> NO MORE MEMORY AVAILABLE\n"); 
		exit(1);
	}

	memcpy(msg->buffer, recvsock->databuf, MAX_DHCP_MSG_LENGTH);

	msg->sent = 0;
	msg->if_index = 0;

	msg->interface_in = recvsock->pkt_interface;
	memcpy(msg->src_addr, recvsock->src_addr, sizeof(recvsock->src_addr));
	msg->datalength = recvsock->buflength;
	msg->pointer_start = msg->buffer;
	msg->dst_addr_type = recvsock->dst_addr_type;
  
	msg->prev = &msg_parser_list;
	msg->next =  msg_parser_list.next;
	msg->prev->next = msg;
	msg->next->prev = msg;
   
	TRACE(dump, "\n%s - RECEIVED NEW MESSAGE ON INTERFACE: %d,  SOURCE: %s\n",
	      dhcp6r_clock(), msg->interface_in, msg->src_addr);
 
	return msg;
}

int
check_buffer(ref, mesg) 
	int ref;
	struct msg_parser *mesg;
{
	int diff;
	diff =(int) (mesg->pstart - mesg->pointer_start);
     
	if ((((int) mesg->datalength) - diff) >= ref)
		return 1;
	else
		return 0;
}

int
put_msg_in_store(mesg) 
	struct msg_parser *mesg;
{
	uint32_t msg_type;
	uint8_t *hop, msg;

	/* --------------------------- */
	mesg->pstart = mesg->buffer;
    
	if (check_buffer(MESSAGE_HEADER_LENGTH, mesg) == 0) {
		printf("put_msg_in_store()--> opt_length has 0 value for "
		       "MESSAGE_HEADER_LENGTH, DROPING... \n");
		return 0;
	}
	msg_type = *((uint32_t *) mesg->pstart);
	msg_type =  (ntohl(msg_type) & 0xFF000000)>>24;

	if (msg_type == SOLICIT) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'SOLICIT' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == REBIND) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;  
                
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'REBIND' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == INFORMATION_REQUEST) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'INFORMATION_REQUEST' FROM CLIENT---> "
		      "IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == REQUEST) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(), 
		      "GOT MESSAGE 'REQUEST' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == REPLY) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'REPLY' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == RENEW) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'RENEW' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == RECONFIGURE) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'RECONFIGURE' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	} 
	else if (msg_type == CONFIRM) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'CONFIRM' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == ADVERTISE) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'ADVERTISE' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	} 
	else if (msg_type == DECLINE) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'DECLINE' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	else if (msg_type == RELEASE) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'RELEASE' FROM CLIENT---> IS TO BE RELAYED\n");
		mesg->isRF = 0;
		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}
	msg = *mesg->pstart;
 
	if (msg == RELAY_FORW) {
		if (check_interface_semafor(mesg->interface_in) == 0)
			return 0;
         
		TRACE(dump, "%s - %s", dhcp6r_clock(),
		      "GOT MESSAGE 'RELAY_FORW' FROM RELAY AGENT---> "
		      "IS TO BE FURTHER RELAYED\n");
		hop = (mesg->pstart + 1);
		if (*hop >= HOP_COUNT_LIMIT) {
			TRACE(dump, "%s - %s", dhcp6r_clock(),
			      "HOP COUNT EXCEEDED, PACKET WILL BE DROPED...\n");
			return 0;
		}

		mesg->hop_count = *hop;
		mesg->isRF = 1;

		if (process_RELAY_FORW(mesg) == 0)
			return 0;

		return 1;
	}

	if (msg == RELAY_REPL) {
		TRACE(dump, "%s - %s", dhcp6r_clock(), 
		      "GOT MESSAGE 'RELAY_REPL' FROM RELAY AGENT OR SERVER---> "
		      "IS TO BE FURTHER RELAYED\n");

		if (process_RELAY_REPL(mesg) == 0)
			return 0;
       
		return 1;
	}

	TRACE(dump, "%s - %s", dhcp6r_clock(),
	      "put_msg_in_store()--> ERROR GOT UNKNOWN MESSAGE DROPING IT......\n");

	return 0;
}
