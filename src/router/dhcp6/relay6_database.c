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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "relay6_database.h"

void  
init_relay(void)
{
	nr_of_uni_addr = 0;	
	multicast_off = 0;
	nr_of_devices = 0;
	max_count = 0;

	cifaces_list.next = &cifaces_list;

	sifaces_list.next = &sifaces_list;
 
	server_list.next = &server_list;
 
	IPv6_address_list.next = &IPv6_address_list;
 
	IPv6_uniaddr_list.next = &IPv6_uniaddr_list;
 
	interface_list.prev = &interface_list;	
	interface_list.next = &interface_list;	

	msg_parser_list.prev = &msg_parser_list;	
	msg_parser_list.next = &msg_parser_list; 
}

int 
check_interface_semafor(int index)
{
	struct interface *device = NULL;
	struct cifaces *iface;

	device = get_interface(index);
	if (device == NULL) {
		printf("FATAL ERROR IN CheckInterfaceSemafor()\n");
		exit(1);   	
	}	
     
	if (cifaces_list.next == &cifaces_list)
		return 1;
          
	for (iface = cifaces_list.next; iface != &cifaces_list; 
	     iface = iface->next) {
		if (strcmp(device->ifname, iface->ciface) == 0)
			return 1;   		
	}	
 
	return 0;
}	

struct interface *get_interface(int if_index)
{
	struct interface *deviface;
 
	for (deviface = interface_list.next; deviface != &interface_list;  
	     deviface = deviface->next) {
		if (deviface->devindex == if_index)
			return deviface;
	}

	return NULL;
}

struct interface *get_interface_s(char *s)
{
	struct interface *deviface;

	for (deviface = interface_list.next; deviface != &interface_list;  
	     deviface = deviface->next) {   
		if (strcmp(s, deviface->ifname)== 0)
			return deviface;
	}

	return NULL;
}

struct msg_parser *get_send_messages_out(void)
{
	struct msg_parser *msg;

	for (msg = msg_parser_list.next; msg != &msg_parser_list; msg = msg->next) {
		if (msg->sent == 0)
			return msg;
	}
         	         
	return NULL;
}


void delete_messages(void)
{
	struct msg_parser *msg;

	for (msg = msg_parser_list.next; msg != &msg_parser_list; msg = msg->next) {
		if (msg->sent == 1) {
			msg->prev->next = msg->next;
			msg->next->prev = msg->prev;
			msg->next = NULL;
			msg->prev = NULL;
			free(msg->buffer);            	 
			free(msg);
			msg = msg_parser_list.next;
		}        	
	}
}


int
process_RELAY_FORW(struct msg_parser *msg)
{
	uint8_t *head = (uint8_t *) malloc(HEAD_SIZE*sizeof(uint8_t));
	uint8_t *newbuff = (uint8_t *) malloc(MAX_DHCP_MSG_LENGTH*sizeof(uint8_t));
	uint8_t *pointer;
	struct interface *device = NULL;
	struct sockaddr_in6 sap;
	int check = 0;
	uint16_t *p16, *optl;
	uint32_t *p32;
	int len, hop;

	if ((head == NULL) || (newbuff == NULL)) {
		printf("ProcessRELAYFORW--> ERROR, NO MORE MEMRY AVAILABLE  \n");	
		exit(1);	
	}	
 
	memset(head, 0, HEAD_SIZE);

	pointer = head;


	if (msg->isRF == 1) { /* got message from a relay agent to be relayed */
		(*pointer) = RELAY_FORW;
		pointer += 1;
		(*pointer) = msg->hop_count + 1; /* increased hop-count */
		msg->hc_pointer = pointer;

		if (max_count == 1) {
			(*pointer) = MAXHOPCOUNT;
			hop = (int) (*pointer);            
			TRACE(dump, "%s - %s%d\n", dhcp6r_clock(), "HOPCOUNT: ", hop);
		}

		pointer += 1;

	}
	else {            /*got message from a client to be relayed */
		(*pointer) = RELAY_FORW;
		pointer += 1;
		(*pointer) = 0; /* hop-count */
		msg->hc_pointer = pointer;

		if (max_count == 1) {
			(*pointer) = MAXHOPCOUNT;
			hop = (int) (*pointer);	            
			TRACE(dump, "%s - %s%d\n", dhcp6r_clock(), "HOPCOUNT: ", hop);
		}
		pointer += 1;
	}

	msg->msg_type = RELAY_FORW;

	device = get_interface(msg->interface_in);
	if (device == NULL) {
		printf("ProcessRELAYFORW--->ERROR NO INTERFACE FOUND!\n");
		exit(1);
	}

	/* fill in link-address */

	if (inet_pton(AF_INET6, msg->src_addr , &sap.sin6_addr) <= 0) {
		printf("ProcessRELAYFORW1--->ERROR IN  inet_pton !\n");
		exit(1);
	}

	if ((!IN6_IS_ADDR_LINKLOCAL(&sap.sin6_addr)) && (nr_of_devices == 1 )) {
		memset(&sap.sin6_addr, 0, sizeof(sap.sin6_addr));
		memcpy(pointer, &sap.sin6_addr, INET6_LEN);
		pointer += INET6_LEN;
	}
	else {
		check = 0;
          
		memset(&sap.sin6_addr, 0, sizeof(sap.sin6_addr));

		if (inet_pton(AF_INET6, device->ipv6addr->gaddr, &sap.sin6_addr) <= 0) {
			printf("ProcessRELAYFORW1--->ERROR IN  inet_pton !\n");
			exit(1);
		}

		memcpy(pointer, &sap.sin6_addr, INET6_LEN);
		pointer += INET6_LEN;
	}


	/* fill in peer-addrees */
	memset(&sap.sin6_addr, 0, sizeof(sap.sin6_addr));

	if (inet_pton(AF_INET6, msg->src_addr , &sap.sin6_addr) <= 0) {
		printf("ProcessRELAYFORW--->ERROR2 IN  inet_pton !\n");
		exit(1);
	}

	memcpy(pointer, &sap.sin6_addr, INET6_LEN);
	pointer += INET6_LEN;

	/* Insert Interface_ID option to identify the interface */
	p16 = (uint16_t *) pointer;
	*p16 = htons(OPTION_INTERFACE_ID);
	pointer += 2;
	p16 = (uint16_t *) pointer;
	*p16 = htons(4); /* 4 octeti length */
	pointer += 2;
	p32 = (uint32_t *) pointer;
	*p32 = htonl(device->opaq);
	pointer += 4;

	p16 = (uint16_t *) pointer;
	*p16 = htons(OPTION_RELAY_MSG);
	pointer += 2;
	optl = (uint16_t *) pointer;
	pointer += 2;
	*optl = htons(msg->datalength);

	len = (pointer - head);
	TRACE(dump, "%s - %s%d\n", dhcp6r_clock(), "RELAY_FORW HEADERLENGTH: ", 
	      len);
	TRACE(dump, "%s - %s%d\n", dhcp6r_clock(), "ORIGINAL MESSAGE LENGTH: ",
	      msg->datalength );
	
	if ((len+ msg->datalength) > MAX_DHCP_MSG_LENGTH) {
		printf(" ERROR FRAGMENTATION WILL OCCUR IF SENT, DROP THE PACKET"
		       "......\n");
		return 0;
	}

	pointer = newbuff;
	memset(pointer, 0, MAX_DHCP_MSG_LENGTH);
	memcpy(pointer, head, len);
	pointer += len;
	memcpy(pointer, msg->buffer, msg->datalength);
	msg->datalength += len; /* final length for sending */
	free(msg->buffer);
	free(head);
	msg->buffer = newbuff;

	return 1;
}

int 
process_RELAY_REPL(struct msg_parser *msg)
{
	uint8_t *newbuff = (uint8_t *) malloc(MAX_DHCP_MSG_LENGTH*sizeof(uint8_t)); 
	uint8_t *pointer, *pstart, *psp;
	struct interface *device = NULL;
	struct sockaddr_in6 sap;
	int check = 0;
	uint16_t *p16, option, opaqlen, msglen;
	uint32_t *p32;
	int len, opaq;
	struct IPv6_address *ipv6a;
	char *s; 
 
	if (newbuff == NULL) {
		printf("ProcessRELAYREPL--> ERROR, NO MORE MEMRY AVAILABLE  \n");	
		exit(1);	
	}

	pointer = msg->buffer;
	pstart = pointer;

	if (( ((int) msg->buffer) - ((int) (pointer - pstart)) ) < 
	    MESSAGE_HEADER_LENGTH ) {
		printf("ProcessRELAYREPL()--> opt_length has 0 value for "
		       "MESSAGE_HEADER_LENGTH, DROPING... \n");
		return 0;
	}

	if (*pointer != RELAY_REPL)
		return 0;

	pointer += 1;  /* RELAY_FORW */
	msg->hop = *pointer;
	pointer += 1;    /* hop-count */
	msg->msg_type = RELAY_REPL;

	if (( ((int) msg->buffer) - ((int) (pointer - pstart)) ) < (2*INET6_LEN) ) {
		printf("ProcessRELAYREPL()--> opt_length has 0 value for "
		       "INET6_LEN, DROPING... \n");
		return 0;
	}

	/* extract link_address */
	memset(msg->link_addr, 0, INET6_ADDRSTRLEN);
	memset(&sap.sin6_addr, 0, sizeof(sap.sin6_addr));
	memcpy(&sap.sin6_addr, pointer, INET6_LEN);
	pointer += INET6_LEN;

	if (inet_ntop(AF_INET6, &sap.sin6_addr, msg->link_addr, 
	              INET6_ADDRSTRLEN ) <=0 ) {
		printf("ProcessRELAYREPL1--->ERROR IN inet_ntop  !\n");
		exit(1);
	}

	/* extract peer address */
	memset(msg->peer_addr, 0, INET6_ADDRSTRLEN);
	memset(&sap.sin6_addr, 0, sizeof(sap.sin6_addr));
	memcpy(&sap.sin6_addr, pointer, INET6_LEN);
	pointer += INET6_LEN;

	if (inet_ntop(AF_INET6, &sap.sin6_addr, msg->peer_addr, 
	              INET6_ADDRSTRLEN ) <=0 ) {         
		printf("ProcessRELAYREPL1--->ERROR IN inet_ntop  !\n");
		exit(1);
	}

	if (( ((int) msg->buffer) - ((int) (pointer - pstart)) ) < 
	    MESSAGE_HEADER_LENGTH ) {
		printf("ProcessRELAYREPL()--> opt_length has 0 value for "
		       "MESSAGE_HEADER_LENGTH, DROPING... \n");
		return 0;
	}

	p16 = (uint16_t *) pointer;
	option = ntohs(*p16);

	if (option == OPTION_INTERFACE_ID) {
		pointer += 2;
		p16 = (uint16_t *) pointer;
		opaqlen = ntohs(*p16);
		pointer += 2;

		if (( ((int) msg->buffer) - ((int) (pointer - pstart)) ) <  opaqlen) {
			printf("ProcessRELAYREPL()--> opt_length has 0 value for "
			       "opaqlen, DROPING... \n");
			return 0;
		}

		p32 = (uint32_t *) pointer;
		opaq = ntohl(*p32);
		pointer += opaqlen;

		if (( ((int) msg->buffer) - ((int) (pointer - pstart)) ) < 
		    MESSAGE_HEADER_LENGTH ) {
			printf("ProcessRELAYREPL()--> opt_length has 0 value for "
			       "MESSAGE_HEADER_LENGTH, DROPING... \n");
			return 0;
		}

		p16 = (uint16_t *) pointer;
		option = ntohs(*p16);

		if (option == OPTION_RELAY_MSG) {
			pointer += 2;
			p16 = (uint16_t *) pointer;
			msglen = ntohs(*p16);
			pointer += 2;
			if (( ((int) msg->buffer) - ((int)(pointer - pstart)) ) < msglen ) {
				printf("ProcessRELAYREPL()--> opt_length has 0 value for "
				       "msglen, DROPING... \n");
				return 0;
			}
              
			/*--------------------------*/
			if (*pointer == RELAY_FORW)
				*pointer = RELAY_REPL; /* is the job of the server to set to 
				                         RELAY_REPL? */
			/*--------------------------*/
			for (device = interface_list.next; device != &interface_list;  
			     device = device->next) {
				if (device->opaq == opaq)
					break;
			}

			if (device != &interface_list ) {
				msg->if_index = device->devindex;
				memset(newbuff, 0, MAX_DHCP_MSG_LENGTH);
				len = (pointer - msg->buffer);
				len = (msg->datalength - len);
				memcpy(newbuff, pointer, len);
				msg->datalength = len;
				free(msg->buffer);
				msg->buffer = newbuff;
				return 1;
				
			}
			else {
				s = msg->link_addr;
                    
				for (device = interface_list.next; device != &interface_list;  
				     device = device->next) {
					ipv6a = device->ipv6addr;

					while(ipv6a!= NULL) { 
						if (strcmp(s, ipv6a->gaddr) == 0) {
							msg->if_index = device->devindex;
							check = 1;
							break;
						}
						ipv6a = ipv6a->next;
					}

					if (check == 1) 
						break;
				}

				if (check == 0) {
					printf("ProcessRELAYREPL--->ERROR NO INTERFACE FOUND!\n");
					return 0;
				}

				memset(newbuff, 0, MAX_DHCP_MSG_LENGTH);
				len = (pointer - msg->buffer);
				len = (msg->datalength - len);
				memcpy(newbuff, pointer, len);
				msg->datalength = len;
				free(msg->buffer);
				msg->buffer = newbuff;
				return 1;	       
			}
		}
		else { /* OPTION_RELAY_MSG */
			printf("ProcessRELAYREPL--->ERROR MESSAGE IS MALFORMED NO "
			       "OPTION_RELAY_MSG FOUND, DROPING...!\n");
			return 0;
		}
	} /* OPTION_INTERFACE_ID */

	if (option == OPTION_RELAY_MSG) {
		pointer += 2;
		p16 = (uint16_t *) pointer;
		msglen = ntohs(*p16);
		pointer += 2;

		if (( ((int) msg->buffer) - ((int) (pointer - pstart)) ) < msglen ) {
			printf("ProcessRELAYREPL()--> opt_length has 0 value for "
			       "msglen, DROPING... \n");
			return 0;
		}

		opaq = 0;
		psp = (pointer + msglen); /* jump over message, seek for 
		                              OPTION_INTERFACE_ID */

		if (( ((int) msg->buffer) - ((int) (psp - pstart)) ) >= 
		    MESSAGE_HEADER_LENGTH ) {
			if (option == OPTION_INTERFACE_ID) {
				psp += 2;
				p16 = (uint16_t *) psp;
				opaqlen = ntohs(*p16);
				psp += 2;

				if (( ((int) msg->buffer) - ((int) (psp - pstart)) ) <  
				    opaqlen) {
					printf("ProcessRELAYREPL()--> opt_length has 0 value "
					       "for opaqlen, DROPING... \n");
					return 0;
				}

				p32 = (uint32_t *) psp;
				opaq = ntohl(*p32);
				psp += opaqlen;
			}
		}

		/*--------------------------*/
		if (*pointer == RELAY_FORW)
			*pointer = RELAY_REPL; /* is the job of the server to set to 
		                          RELAY_REPL? */
		/*--------------------------*/
		for (device = interface_list.next; device != &interface_list;
		     device = device->next) {             
			if (device->opaq == opaq)
				break;
		}

		if (device != &interface_list) {
			msg->if_index = device->devindex;
			memset(newbuff, 0, MAX_DHCP_MSG_LENGTH);
			len = (pointer - msg->buffer);
			len = (msg->datalength - len);
			memcpy(newbuff, pointer, len);
			msg->datalength = len;
			free(msg->buffer);
			msg->buffer = newbuff;
			return 1;
		}
		else {
			s = msg->link_addr;
	
			for (device = interface_list.next; device != &interface_list;
		     	device = device->next) {
				ipv6a = device->ipv6addr;
                   	  	
				while(ipv6a != NULL) { 
					if (strcmp(s, ipv6a->gaddr) == 0) {
						msg->if_index = device->devindex;
						check = 1;
						break;
					}
					ipv6a = ipv6a->next;
				}

				if (check == 1) 
					break;
			}
  
			if (check == 0) {
				printf("ProcessRELAYREPL--->ERROR NO INTERFACE FOUND!\n");
				return 0;
			}

			memset(newbuff, 0, MAX_DHCP_MSG_LENGTH);
			len = (pointer - msg->buffer);
			len = (msg->datalength - len);
			memcpy(newbuff, pointer, len);
			msg->datalength = len;
			free(msg->buffer);
			msg->buffer = newbuff;
			return 1;
		}
	}
	else { /* OPTION_RELAY_MSG */
		printf("ProcessRELAYREPL--->ERROR MESSAGE IS MALFORMED NO "
		       "OPTION_RELAY_MSG FOUND, DROPING...!\n");
		return 0;
	}

	return 1;
}
