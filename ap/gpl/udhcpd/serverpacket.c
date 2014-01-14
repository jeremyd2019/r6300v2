/* serverpacket.c
 *
 * Constuct and send DHCP server packets
 *
 * Russ Dill <Russ.Dill@asu.edu> July 2001
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

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>

#include "packet.h"
#include "debug.h"
#include "dhcpd.h"
#include "options.h"
#include "leases.h"

#include <sys/sysinfo.h>
#include <stdlib.h>

#define GUEST_LEASE_TIME	1800	//secs = 30 minutes
#define MAX_REFRESH_TIME	3		// 3 secs, to refresh the guest assoc list
#define GUEST_ASSOC_LIST	"/tmp/wlan_guest_clients"

/* 
 * Declare a buffer to store wireless assoclist.
 * response of "wl assoclist":
 * 
 *  assoclist 00:11:22:33:44:55
 *  assoclist 00:22:33:44:55:66
 *  ...
 *
 * To accept max 64 clients, we need (27+2) * 64 = 1856.
 * So choose 2048 buf size to store all guest clients.
 */
static char guest_assoclist[2048];

static int get_guest_assoclist(void)
{
	static long last_update =0;
	struct sysinfo info;
	char command[128];
	FILE *fp = NULL;
    
	sysinfo(&info);
	if (info.uptime - last_update >= MAX_REFRESH_TIME) {
		/* Issue wl commands to create the new list */
		sprintf(command, "wl -i wl0.1 assoclist > %s", GUEST_ASSOC_LIST);
		system(command);
		sprintf(command, "wl -i wl1.1 assoclist >> %s", GUEST_ASSOC_LIST);
		system(command);

		/* Read the command output to internal buffer */
		memset(guest_assoclist, 0, sizeof(guest_assoclist));
		fp = fopen(GUEST_ASSOC_LIST, "r");
		if (fp) {
			fread(guest_assoclist, 1, sizeof(guest_assoclist), fp);
			fclose(fp);
			last_update = info.uptime;
		}
	}
}

static int is_guest_network(char *mac)
{
	char client_mac[32];

	sprintf(client_mac, "%02X:%02X:%02X:%02X:%02X:%02X", 
						(unsigned char)mac[0],
						(unsigned char)mac[1],
						(unsigned char)mac[2],
						(unsigned char)mac[3],
						(unsigned char)mac[4],
						(unsigned char)mac[5]);
	get_guest_assoclist();
	if (strstr(guest_assoclist, client_mac))
		return 1;
	else
		return 0;
}
/* send a packet to giaddr using the kernel ip stack */
static int send_packet_to_relay(struct dhcpMessage *payload)
{
	DEBUG(LOG_INFO, "Forwarding packet to relay");

	return kernel_packet(payload, server_config.server, SERVER_PORT,
			payload->giaddr, SERVER_PORT);
}


/* send a packet to a specific arp address and ip address by creating our own ip packet */
static int send_packet_to_client(struct dhcpMessage *payload, int force_broadcast)
{
	unsigned char *chaddr;
	u_int32_t ciaddr;
	
	if (force_broadcast) {
		DEBUG(LOG_INFO, "broadcasting packet to client (NAK)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else if (payload->ciaddr) {
		DEBUG(LOG_INFO, "unicasting packet to client ciaddr");
		ciaddr = payload->ciaddr;
		chaddr = payload->chaddr;
	} else if (ntohs(payload->flags) & BROADCAST_FLAG) {
		DEBUG(LOG_INFO, "broadcasting packet to client (requested)");
		ciaddr = INADDR_BROADCAST;
		chaddr = MAC_BCAST_ADDR;
	} else {
		DEBUG(LOG_INFO, "unicasting packet to client yiaddr");
		ciaddr = payload->yiaddr;
		chaddr = payload->chaddr;
	}
	return raw_packet(payload, server_config.server, SERVER_PORT, 
			ciaddr, CLIENT_PORT, chaddr, server_config.ifindex);
}


/* send a dhcp packet, if force broadcast is set, the packet will be broadcast to the client */
static int send_packet(struct dhcpMessage *payload, int force_broadcast)
{
	int ret;

	if (payload->giaddr)
		ret = send_packet_to_relay(payload);
	else ret = send_packet_to_client(payload, force_broadcast);
	return ret;
}


static void init_packet(struct dhcpMessage *packet, struct dhcpMessage *oldpacket, char type)
{
	init_header(packet, type);
	packet->xid = oldpacket->xid;
	memcpy(packet->chaddr, oldpacket->chaddr, 16);
	packet->flags = oldpacket->flags;
	packet->giaddr = oldpacket->giaddr;
	packet->ciaddr = oldpacket->ciaddr;
	add_simple_option(packet->options, DHCP_SERVER_ID, server_config.server);
}


/* add in the bootp options */
static void add_bootp_options(struct dhcpMessage *packet)
{
	packet->siaddr = server_config.siaddr;
	if (server_config.sname)
		strncpy(packet->sname, server_config.sname, sizeof(packet->sname) - 1);
	if (server_config.boot_file)
		strncpy(packet->file, server_config.boot_file, sizeof(packet->file) - 1);
}


/* send a DHCP OFFER to a DHCP DISCOVER */
int sendOffer(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct dhcpOfferedAddr *lease = NULL;
	u_int32_t req_align, lease_time_align = server_config.lease;
	unsigned char *req, *lease_time;
	struct option_set *curr;
	struct in_addr addr;
    unsigned char mac[6];
	u_int32_t reserved_ip;

    memcpy(mac, oldpacket->chaddr, 6);
    
	init_packet(&packet, oldpacket, DHCPOFFER);
	
	/* ADDME: if static, short circuit */
	/* the client is in our lease/offered table */
	if ((lease = find_lease_by_chaddr(oldpacket->chaddr)) &&
        
        /* Make sure the IP is not already used on network */
        !check_ip(lease->yiaddr)) {

		if (!lease_expired(lease)) 
			lease_time_align = lease->expires - time(0);
		packet.yiaddr = lease->yiaddr;
		
    /* Find a reserved ip for this MAC */
	} else if ( (reserved_ip = find_reserved_ip(mac)) != 0) {
		packet.yiaddr = htonl(reserved_ip);
        
	/* Or the client has a requested ip */
	} else if ((req = get_option(oldpacket, DHCP_REQUESTED_IP)) &&

		/* Don't look here (ugly hackish thing to do) */
		memcpy(&req_align, req, 4) &&

        /* check if the requested ip has been reserved */
        check_reserved_ip(req_align, mac) &&
           
		/* and the ip is in the lease range */
		ntohl(req_align) >= ntohl(server_config.start) &&
		ntohl(req_align) <= ntohl(server_config.end) &&

         /* Check that this request ip is not on network */
         //!check_ip(ntohl(req_align)) &&
	 /* the input parameter of check_ip() should be network order */
	 !check_ip(req_align) && /*  modified by Max Ding, 07/07/2011 @TD #42 of WNR3500Lv2 */
           
		/* and its not already taken/offered */ /* ADDME: check that its not a static lease */
		((!(lease = find_lease_by_yiaddr(req_align)) ||
		   
		/* or its taken, but expired */ /* ADDME: or maybe in here */
		lease_expired(lease)))) {
		    packet.yiaddr = req_align; 

	/* otherwise, find a free IP */ /*ADDME: is it a static lease? */
	} else {
		packet.yiaddr = find_address2(0, mac);
		
		/* try for an expired lease */
		if (!packet.yiaddr) packet.yiaddr = find_address2(1, mac);
	}
	
	if(!packet.yiaddr) {
		LOG(LOG_WARNING, "no IP addresses to give -- OFFER abandoned");
		return -1;
	}
	
	if (!add_lease(packet.chaddr, packet.yiaddr, server_config.offer_time)) {
		LOG(LOG_WARNING, "lease pool is full -- OFFER abandoned");
		return -1;
	}		

	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config.lease) 
			lease_time_align = server_config.lease;
	}

	/* Make sure we aren't just using the lease time from the previous offer */
	if (lease_time_align < server_config.min_lease) 
		lease_time_align = server_config.lease;
	if (is_guest_network(mac)) {
		lease_time_align = GUEST_LEASE_TIME;
		DEBUG(LOG_INFO, "send OFFER to guest network client with lease time %d sec", GUEST_LEASE_TIME);
	}
	/* ADDME: end of short circuit */		
	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));

	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);
	
	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending OFFER of %s", inet_ntoa(addr));
	return send_packet(&packet, 0);
}


int sendNAK(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;

	init_packet(&packet, oldpacket, DHCPNAK);
	
	DEBUG(LOG_INFO, "sending NAK");
	return send_packet(&packet, 1);
}


int sendACK(struct dhcpMessage *oldpacket, u_int32_t yiaddr)
{
	struct dhcpMessage packet;
	struct option_set *curr;
	unsigned char *lease_time;
	
	// added start Bob Guo 11/15/2006
	FILE *fp;
	unsigned char *client_mac, *client_ip;
	char logBuffer[96];
	// added end Bob Guo 11/15/2006
	
	u_int32_t lease_time_align = server_config.lease;
	struct in_addr addr;

	init_packet(&packet, oldpacket, DHCPACK);
	packet.yiaddr = yiaddr;
	
	if ((lease_time = get_option(oldpacket, DHCP_LEASE_TIME))) {
		memcpy(&lease_time_align, lease_time, 4);
		lease_time_align = ntohl(lease_time_align);
		if (lease_time_align > server_config.lease) 
			lease_time_align = server_config.lease;
		else if (lease_time_align < server_config.min_lease) 
			lease_time_align = server_config.lease;
	}
	
	if (is_guest_network(packet.chaddr)) {
		lease_time_align = GUEST_LEASE_TIME;
		DEBUG(LOG_INFO, "send ACK to guest network client with lease time %d sec", GUEST_LEASE_TIME);
	}
	add_simple_option(packet.options, DHCP_LEASE_TIME, htonl(lease_time_align));
	
	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	addr.s_addr = packet.yiaddr;
	LOG(LOG_INFO, "sending ACK to %s", inet_ntoa(addr));

	if (send_packet(&packet, 0) < 0) 
		return -1;

	add_lease(packet.chaddr, packet.yiaddr, lease_time_align);
	// added start Bob Guo 11/15/2006
	client_mac = (unsigned char *)packet.chaddr;
	client_ip = (unsigned char *)&packet.yiaddr;
	sprintf(logBuffer, "[DHCP IP: (%d.%d.%d.%d)] to MAC address %02X:%02X:%02X:%02X:%02X:%02X,",
	                                          *client_ip, *(client_ip+1), *(client_ip+2), *(client_ip+3),
	                                          *client_mac, *(client_mac+1), *(client_mac+2), *(client_mac+3), *(client_mac+4), *(client_mac+5));
	if ((fp = fopen("/dev/aglog", "r+")) != NULL)
  {
        fwrite(logBuffer, sizeof(char), strlen(logBuffer)+1, fp);
        fclose(fp);
  }	
	// added end Bob Guo 11/15/2006

	return 0;
}


int send_inform(struct dhcpMessage *oldpacket)
{
	struct dhcpMessage packet;
	struct option_set *curr;

	init_packet(&packet, oldpacket, DHCPACK);
	
	curr = server_config.options;
	while (curr) {
		if (curr->data[OPT_CODE] != DHCP_LEASE_TIME)
			add_option_string(packet.options, curr->data);
		curr = curr->next;
	}

	add_bootp_options(&packet);

	return send_packet(&packet, 0);
}



