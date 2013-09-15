/* reserveip.c
 *
 * udhcp Server
 * Copyright (C) 2005 Eric Huang <eric.sy.huang@.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"

int num_of_reservedIP=0;
char resrvMacAddr[MAX_RESERVED_MAC][MAX_TOKEN_SIZE];
char resrvIpAddr[MAX_RESERVED_IP][MAX_TOKEN_SIZE];

/*
 * Strip the ':'s from the MAC address
 */
static void trimMacAddr(char *macAddr)
{
    #define C_MAX_MACADDR_LEN       18
    char temp[C_MAX_MACADDR_LEN];
    char *var = macAddr;
    int i = 0;

    while (*var)
    {
        if (*var != ':')
        {
            temp[i++] = *var;
            if (i == C_MAX_MACADDR_LEN-1)
            break;
        }
            var++;
    }
    temp[i] = '\0';

    strcpy(macAddr, temp);
}

static void HexToAscii (unsigned char *hexKey, unsigned char *asciiKey)
{
    unsigned char tmp[32], Data=0, i;
    for (i=0; i<strlen(hexKey); i++)
    {
        tmp[i] = hexKey[i];
        if (tmp[i] >= '0' && tmp[i] <= '9')
            Data = Data*0x10 + tmp[i] - 0x30;
        else if (tmp[i] >= 'A' && tmp[i] <= 'F')
            Data = Data*0x10 + tmp[i] - 0x37;
        else if (tmp[i] >= 'a' && tmp[i] <= 'f')
        {

            Data = Data*0x10 + tmp[i] - 0x57;
        }
        if ((i % 2) == 1) {
            asciiKey[i/2] = Data;
            Data = 0;
        }
    }
    asciiKey[strlen(hexKey)/2]='\0';
}

static int getTokens(char *str, char *delimiter, char token[][MAX_TOKEN_SIZE], int maxNumToken)
{
    char temp[16*1024];    
    char *field;
    int numToken=0, i, j;
    char *ppLast = 0;

    if (strchr(str, '\n')) *(strchr(str, '\n')) = '\0';
    
    /* Check for empty string */
    if (str == 0 || str[0] == '\0')
    {
        return 0;
    }
   
    /* Now get the tokens */
    strcpy(temp, str);
    
    for (i=0; i<maxNumToken; i++)
    {
        if (i == 0)
            field = strtok_r(temp, delimiter, &ppLast);
        else 
            field = strtok_r(NULL, delimiter, &ppLast);

        if (field == NULL || field[0] == '\0')
        {
                for (j=i; j<maxNumToken; j++)
                    token[j][0] = '\0';
            break;
        }

        numToken++;
        strcpy(token[i], field);
    }

    return numToken;
}

/*
 * Read the reserved IP addresses from configuration file and put them into array.
 * Returns the number of IP address read.
 */
int getReservedAddr(char reservedMacAddr[][MAX_TOKEN_SIZE], char reservedIpAddr[][MAX_TOKEN_SIZE])
{
#define BUFFER_SIZE     2*1024
    int numReservedMac=0, numReservedIp=0;
    char buffer[BUFFER_SIZE];      /*  modified pling 10/04/2007, to allow max 64 reserved IP */
    FILE *in;
    
	if (!(in = fopen("/tmp/udhcpd_resrv.conf", "r"))) 
    {
		LOG(LOG_ERR, "unable to open config file: /tmp/udhcpd_resrv.conf");
		return 0;
	}    
    
    /*get ip*/
    memset(buffer, 0, BUFFER_SIZE);
    if ( fgets(buffer, BUFFER_SIZE, in) )
    {
        numReservedIp = getTokens(buffer, " ", reservedIpAddr, MAX_RESERVED_IP);
        //LOG(LOG_INFO, "# of reserved ip: %d\n", numReservedIp);
    }
    else
    {
        //LOG(LOG_ERR, "reserved ip not found.\n");
        return 0;
    }

    /*get mac*/
    memset(buffer, 0, BUFFER_SIZE);
    if ( fgets(buffer, BUFFER_SIZE, in) )
    {
        numReservedMac = getTokens(buffer, " ", reservedMacAddr, MAX_RESERVED_MAC);
        //LOG(LOG_INFO, "# of reserved mac: %d\n", numReservedIp);
    }
    else
    {
        //LOG(LOG_ERR, "reserved mac not found.\n");
        return 0;
    }

    if (numReservedMac != numReservedIp)
    {
        //LOG(LOG_INFO, "WARNING! Reserved IP inconsistency!\n");
    }

    return (numReservedMac<numReservedIp ? numReservedMac:numReservedIp);
}

int check_reserved_ip(u_int32_t req_ip, u_int8_t *chaddr)
{
    u_int32_t reserved_ip=0;
    int i=0;

    /*  added start by EricHuang, 02/01/2007 */
    //if ( ntohl(req_ip) == server_config.server ) // modified, wenchia, 2007/09/10
    if ( ntohl(req_ip) == ntohl(server_config.server) )
    {
        return 0; /* requested ip is router's ip */
    }
    /*  added end by EricHuang, 02/01/2007 */

    for (i=0; i<num_of_reservedIP; i++)
    {
        reserved_ip = inet_addr(resrvIpAddr[i]);
        /* modify start,Zz Shan@Reserved ip 05/07/2008*/
        //if ( reserved_ip == ntohl(req_ip) )
        if ( reserved_ip == (req_ip) )
        /* modify end,Zz Shan@Reserved ip 05/07/2008*/
        {
            unsigned char tempMac[32], mac[7];
            sprintf(tempMac, "%s", resrvMacAddr[i]);
            trimMacAddr(tempMac);
            HexToAscii(tempMac, mac);
            if ( memcmp(chaddr, mac, 6) == 0 )
            {
                return 1; /*reserved ip and the ip reserved for this mac*/
            }

            return 0; /*reserved ip but not for this mac*/
        }
    }

    return 1; /*not a reserved ip*/
}

u_int32_t find_reserved_ip(u_int8_t *chaddr)
{
    int i;

    for (i=0; i<num_of_reservedIP; i++)
    {
        unsigned char tempMac[32], mac[7];
        sprintf(tempMac, "%s", resrvMacAddr[i]);
        trimMacAddr(tempMac);
        HexToAscii(tempMac, mac);
        if ( memcmp(chaddr, mac, 6) == 0 )
        {
            /*reserved ip found for this mac*/
            return ntohl(inet_addr(resrvIpAddr[i]));
        }
    }

    return 0;   /* Sorry, no reserved ip found */
}
