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

#ifndef __DHCP6R_H_DEFINED
#define __DHCP6R_H_DEFINED

#include <stdio.h>
#include <stdint.h>

#define MAX_DHCP_MSG_LENGTH     1400
#define MESSAGE_HEADER_LENGTH   4
#define ALL_DHCP_SERVERS           "FF05::1:3"
#define ALL_DHCP_RELAY_AND_SERVERS "FF02::1:2"
#define INET6_LEN               16
#define OPAQ                    5000   //opaq value for interface id
#define HEAD_SIZE               400
#define HOP_COUNT_LIMIT         30
#define DUMPFILE                "/var/log/dhcp6r.log"
#define INTERFACEINFO           "/proc/net/if_inet6"
#define PIDFILE                 "/var/run/dhcp6r.pid"
#define MAXHOPCOUNT             32
#define SERVER_PORT             547
#define CLIENT_PORT             546
#define TRACE                   fprintf
  
#define SOLICIT				1
#define ADVERTISE			2
#define REQUEST				3
#define CONFIRM				4
#define RENEW				5
#define REBIND				6
#define REPLY				7
#define RELEASE				8
#define DECLINE				9
#define RECONFIGURE			10
#define INFORMATION_REQUEST	11
#define RELAY_FORW			12
#define RELAY_REPL			13

#define OPTION_RELAY_MSG	9
#define OPTION_INTERFACE_ID	18

char *dhcp6r_clock __P((void));
FILE  *dump;
void  handler __P((int signo));


#endif /* __DHCP6R_H_DEFINED */
