/*
 * pppoe, a PPP-over-Ethernet redirector
 * Copyright (C) 1999 Luke Stras <stras@ecf.toronto.edu>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Revision History
 * 1999/09/22 stras Initial version
 * 1999/09/24 stras Changed header files for greater portability
 * 1999/10/02 stras Added more logging, bug fixes
 * 1999/10/02 mr    Port to bpf/OpenBSD; starvation fixed; efficiency fixes
 * 1999/10/18 stras added BUGGY_AC code, partial forwarding
 */ 

#include <sys/types.h>
/*  wklin added start, 07/26/2007 */
#include <sys/stat.h>
/*  wklin added end, 07/26/2007 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> /* wklin added, 01/10/2007 */
/*  modified start Winster Chan 11/25/2005 */
/* #include <net/if.h> */
#include "pppoe.h"
/*  modified end Winster Chan 11/25/2005 */
#include <sys/sysinfo.h> /*added by EricHuang, 01/11/2007*/

#ifdef __linux__
#include <net/if_arp.h>
#endif /* __linux__ */
#if defined(__GNU_LIBRARY__) && __GNU_LIBRARY__ < 6
#include <linux/if_ether>
#else
#include <netinet/if_ether.h>
#endif

#include <assert.h>
#ifdef __linux__
#include <getopt.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* #include <sys/time.h> */
#include <time.h>
#include <sys/ioctl.h>
#include <sys/wait.h>


#ifdef USE_BPF
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif /* ETH_ALEN */
/* set this to desired size - you may not get this */
int bpf_buf_size = 65536;
#include <net/bpf.h>
#include <fcntl.h>
#include <nlist.h>
#include <kvm.h>
unsigned char local_ether[ETH_ALEN]; /* need to this filter packets */
#endif /* USE_BPF */

#include <errno.h>
#ifdef __linux__
extern int errno;
#endif

/* used as the size for a packet buffer */
/* should be > 2 * size of max packet size */
#define PACKETBUF (4096 + 30)
/*  added start Winster Chan 11/25/2005 */
#define TAGBUF 128
/*  added end Winster Chan 11/25/2005 */

/*  added start, Winster Chan, 06/26/2006 */
static int poxfd = -1;
static int pppfd = -1;
unsigned short sessId = 0;
char dstMac[ETH_ALEN];
/*  added start, Winster Chan, 06/26/2006 */

char *service_name = NULL; /*  wklin added, 03/27/2007 */

#define VERSION_MAJOR 0
#define VERSION_MINOR 3

/* references: RFC 2516 */
/* ETHER_TYPE fields for PPPoE */

#define ETH_P_PPPOE_DISC 0x8863 /* discovery stage */
#define ETH_P_PPPOE_SESS 0x8864 /* session stage */

/* ethernet broadcast address */
#define MAC_BCAST_ADDR "\xff\xff\xff\xff\xff\xff"

/* PPPoE packet; includes Ethernet headers and such */
struct pppoe_packet {
#ifdef __linux__
    struct ethhdr ethhdr; /* ethernet header */
#else
    struct ether_header ethhdr; /* ethernet header */
#endif
    unsigned int ver:4; /* pppoe version */
    unsigned int type:4; /* pppoe type */
    unsigned int code:8; /* pppoe code CODE_* */
    unsigned int session:16; /* session id */
    unsigned short length; /* payload length */
    /* payload follows */
};

/* maximum payload length */
#define MAX_PAYLOAD (1484 - sizeof(struct pppoe_packet))

/* PPPoE codes */
#define CODE_SESS 0x00 /* PPPoE session */
#define CODE_PADI 0x09 /* PPPoE Active Discovery Initiation */
#define CODE_PADO 0x07 /* PPPoE Active Discovery Offer */
#define CODE_PADR 0x19 /* PPPoE Active Discovery Request */
#define CODE_PADS 0x65 /* PPPoE Active Discovery Session-confirmation */
#define CODE_PADT 0xa7 /* PPPoE Active Discovery Terminate */

/* also need */
#define STATE_RUN (-1)

/* PPPoE tag; the payload is a sequence of these */
struct pppoe_tag {
    unsigned short type; /* tag type TAG_* */
    unsigned short length; /* tag length */
    /* payload follows */
} __attribute__ ((packed)); /*added by EricHuang, 07/23/2007*/

/*  added start Winster Chan 11/25/2005 */
#define TAG_STRUCT_SIZE  sizeof(struct pppoe_tag)
#define PPP_PPPOE_SESSION   "/tmp/ppp/pppoe_session"
#define PPP_PPPOE2_SESSION   "/tmp/ppp/pppoe2_session"
/*#define PPP_PPPOE_IFNAME    "/tmp/ppp/pppoe_ifname"*/
/*  added end Winster Chan 11/25/2005 */

/* PPPoE tag types */
#define TAG_END_OF_LIST        0x0000
#define TAG_SERVICE_NAME       0x0101
#define TAG_AC_NAME            0x0102
#define TAG_HOST_UNIQ          0x0103
#define TAG_AC_COOKIE          0x0104
#define TAG_VENDOR_SPECIFIC    0x0105
#define TAG_RELAY_SESSION_ID   0x0110
#define TAG_SERVICE_NAME_ERROR 0x0201
#define TAG_AC_SYSTEM_ERROR    0x0202
#define TAG_GENERIC_ERROR      0x0203

/* globals */
int opt_verbose = 0;   /* logging */
int opt_fwd = 0;       /* forward invalid packets */
int opt_fwd_search = 0; /* search for next packet when forwarding */
#ifdef MULTIPLE_PPPOE
#define log_file stderr
#else
FILE *log_file = NULL;
#endif
FILE *error_file = NULL;
#ifdef MULTIPLE_PPPOE
int ppp_ifunit = 0; /*  wklin added, 08/16/2007 */
#endif
pid_t sess_listen = 0, pppd_listen = 0; /* child processes */
int disc_sock = 0, sess_sock = 0; /* PPPoE sockets */
char src_addr[ETH_ALEN]; /* source hardware address */
char dst_addr[ETH_ALEN]; /* destination hardware address */
char *if_name = NULL; /* interface to use */
int session = 0; /* identifier for our session */
int clean_child = 0; /* flag set when SIGCHLD received */
/*  added start Winster Chan 11/25/2005 */
char pado_tags[TAGBUF]; /* TAGs of PADO */
int pado_tag_size = 0;
typedef struct {
    unsigned short  usPadLen;   /* Tag length in type of unsigned short */
    int             nPadLen;    /* Tag length in type of integer */
    char            *pPadStart; /* Start point of tag payload */
} sPadxTag, *pPadxTag;
/*  added end Winster Chan 11/25/2005 */
#ifdef NEW_WANDETECT
int bWanDetect = 0;/*  added by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#endif

/* Winster Chan debugtest */
#define DEBUG_PRINT_PACKET  0
#define DEBUG_SEND_PACKET   0
#define BUFRING             5 /* 40 */ /*  wklin modified, 08/13/2007 */
#define DEBUG_PRINT         0
#define PPPOE_DEBUG_FILE    "/tmp/ppp/pppoeDbg"
FILE *fp0;
pid_t main_pid, sess_pid, pppd_pid;


typedef struct {
    /* unsigned char *pBufAddr; */
    unsigned char packetBuf[PACKETBUF];
} sPktBuf, *pPktBuf;

void
print_hex(unsigned char *buf, int len)
{
    int i;

    if (opt_verbose == 0)
      return;

    for (i = 0; i < len; i++)
      fprintf(log_file, "%02x ", (unsigned char)*(buf+i));

    fprintf(log_file, "\n");
}

void
print_packet(struct pppoe_packet *p)
{
    int i;
    struct pppoe_tag *t = (struct pppoe_tag*)(p + 1);
    struct pppoe_tag tag; /* needed to avoid alignment problems */
    char *buf;
    time_t tm;

    if (opt_verbose == 0)
	return;

    time(&tm);

    fprintf(log_file, "Ethernet header:\n");
    fprintf(log_file, "h_dest: ");
#ifdef __linux__
    for (i = 0; i < 6; i++)
	fprintf(log_file, "%02x:", (unsigned)p->ethhdr.h_dest[i]);
#else
    for (i = 0; i < 6; i++)
	fprintf(log_file, "%02x:", (unsigned)p->ethhdr.ether_dhost[i]);
#endif
    fprintf(log_file, "\nh_source: ");
#ifdef __linux__
    for (i = 0; i < 6; i++)
	fprintf(log_file, "%02x:", (unsigned)p->ethhdr.h_source[i]);
#else
    for (i = 0; i < 6; i++)
	fprintf(log_file, "%02x:", (unsigned)p->ethhdr.ether_shost[i]);
#endif

#ifdef __linux__
    fprintf(log_file, "\nh_proto: 0x%04x ",
	    (unsigned)ntohs(p->ethhdr.h_proto));
#else
    fprintf(log_file, "\nh_proto: 0x%04x ",
	    (unsigned)ntohs(p->ethhdr.ether_type));
#endif

#ifdef __linux__
    switch((unsigned)ntohs(p->ethhdr.h_proto))
#else
    switch((unsigned)ntohs(p->ethhdr.ether_type))
#endif
    {
        case ETH_P_PPPOE_DISC:
            fprintf(log_file, "(PPPOE Discovery)\n");
            break;
        case ETH_P_PPPOE_SESS:
            fprintf(log_file, "(PPPOE Session)\n");
            break;
        default:
            fprintf(log_file, "(Unknown)\n");
    }

    fprintf(log_file, "PPPoE header: \nver: 0x%01x type: 0x%01x code: 0x%02x "
	   "session: 0x%04x length: 0x%04x ", (unsigned)p->ver,
	   (unsigned)p->type, (unsigned)p->code, (unsigned)p->session,
	   (unsigned)ntohs(p->length));

    switch(p->code)
    {
        case CODE_PADI:
            fprintf(log_file, "(PADI)\n");
            break;
        case CODE_PADO:
            fprintf(log_file, "(PADO)\n");
            break;
        case CODE_PADR:
            fprintf(log_file, "(PADR)\n");
            break;
        case CODE_PADS:
            fprintf(log_file, "(PADS)\n");
            break;
        case CODE_PADT:
            fprintf(log_file, "(PADT)\n");
            break;
        default:
            fprintf(log_file, "(Unknown)\n");
    }

#ifdef __linux__
    if (ntohs(p->ethhdr.h_proto) != ETH_P_PPPOE_DISC)
#else
    if (ntohs(p->ethhdr.ether_type) != ETH_P_PPPOE_DISC)
#endif
    {
        print_hex((unsigned char *)(p+1), ntohs(p->length));
        return;
    }


    while (t < (struct pppoe_tag *)((char *)(p+1) + ntohs(p->length)))
    {
        /* no guarantee in PPPoE spec that t is aligned at all... */
        memcpy(&tag,t,sizeof(tag));
        fprintf(log_file, "PPPoE tag:\ntype: %04x length: %04x ",
            ntohs(tag.type), ntohs(tag.length));
        switch(ntohs(tag.type))
        {
            case TAG_END_OF_LIST:
                fprintf(log_file, "(End of list)\n");
                break;
            case TAG_SERVICE_NAME:
                fprintf(log_file, "(Service name)\n");
                break;
            case TAG_AC_NAME:
                fprintf(log_file, "(AC Name)\n");
                break;
            case TAG_HOST_UNIQ:
                fprintf(log_file, "(Host Uniq)\n");
                break;
            case TAG_AC_COOKIE:
                fprintf(log_file, "(AC Cookie)\n");
                break;
            case TAG_VENDOR_SPECIFIC:
                fprintf(log_file, "(Vendor Specific)\n");
                break;
            case TAG_RELAY_SESSION_ID:
                fprintf(log_file, "(Relay Session ID)\n");
                break;
            case TAG_SERVICE_NAME_ERROR:
                fprintf(log_file, "(Service Name Error)\n");
                break;
            case TAG_AC_SYSTEM_ERROR:
                fprintf(log_file, "(AC System Error)\n");
                break;
            case TAG_GENERIC_ERROR:
                fprintf(log_file, "(Generic Error)\n");
                break;
            default:
                fprintf(log_file, "(Unknown)\n");
        }
        if (ntohs(tag.length) > 0) {
            switch (ntohs(tag.type))
            {
                case TAG_SERVICE_NAME:
                case TAG_AC_NAME:
                case TAG_SERVICE_NAME_ERROR:
                case TAG_AC_SYSTEM_ERROR:
                case TAG_GENERIC_ERROR: /* ascii data */
                    buf = malloc(ntohs(tag.length) + 1);
                    memset(buf, 0, ntohs(tag.length)+1);
                    strncpy(buf, (char *)(t+1), ntohs(tag.length));
                    buf[ntohs(tag.length)] = '\0';
                    fprintf(log_file, "data (UTF-8): %s\n", buf);
                    free(buf);
                    break;

                case TAG_HOST_UNIQ:
                case TAG_AC_COOKIE:
                case TAG_RELAY_SESSION_ID:
                    fprintf(log_file, "data (bin): ");
                    for (i = 0; i < ntohs(tag.length); i++)
                        fprintf(log_file, "%02x", (unsigned)*((char *)(t+1) + i));
                    fprintf(log_file, "\n");
                    break;

                default:
                    fprintf(log_file, "unrecognized data\n");
            }
            t = (struct pppoe_tag *)((char *)(t+1)+ntohs(tag.length));
	}
    }
}

/*  added start, Winster Chan, 06/26/2006 */
#ifndef MULTIPLE_PPPOE
/**************************************************************************
** Function:    addr_itox()
** Description: Convert the <int> address value getting from file to
**                  <unsigned char> address type.
** Parameters:  (unsigned char *) daddr -- destination address value
**              (int *) saddr -- source address value
**              (int) convlen -- convert length
** Return:      none.
**************************************************************************/
static void addr_itox(unsigned char *daddr, int *saddr, int convlen)
{
    int i;

    for (i = 0; i < convlen; i++)
        daddr[i] = (unsigned char)saddr[i];
}
#endif
/**************************************************************************
** Function:    pptp_pppox_open()
** Description: Open socket to kernel pppox driver, and open ppp device
** Parameters:  (int *) poxfd -- pointer of file descriptor for pppox
**              (int *) pppfd -- pointer of file descriptor for ppp device
** Return:      none.
**************************************************************************/
void pptp_pppox_open(int *poxfd, int *pppfd)
{
#ifdef MULTIPLE_PPPOE
    /* Open socket to pppox kernel module */
    
    *poxfd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_OE);
    if (*poxfd >= 0) 
    {
        /* Open ppp device */
        *pppfd = open("/dev/ppp", O_RDWR);
	    if (*pppfd < 0) 
	    { /* on error */
	        fprintf(stderr, "pppoe: error opening pppfd.\n");
	        close(*poxfd);
	    } 
	    else
	        return;
    } 
    else 
    {
	    fprintf(stderr, "pppoe: error opening poxfd.\n");
    }
    *poxfd = -1;
    *pppfd = -1;
#else
    /* Open socket to pppox kernel module */
    *poxfd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_OE);
    if (*poxfd >= 0) {
        /* Open ppp device */
        *pppfd = open("/dev/ppp", O_RDWR);
    }
    else {
        *poxfd = -1;
        *pppfd = -1;
    }
#endif
}

/**************************************************************************
** Function:    pptp_pppox_get_info()
** Description: Get the essential information for connecting pptp kernel
**                  module. Such as Source IP, Destination IP, Remote MAC
**                  address, Device name, call_id, and peer_call_id, etc.
** Parameters:  none.
** Return:      (struct sockaddr_pppox)sp_info -- structure of information.
**************************************************************************/
struct sockaddr_pppox pptp_pppox_get_info(void)
{
    struct sockaddr_pppox sp_info;
    char devName[16];
    strcpy(devName, if_name);
    memset(&sp_info, 0, sizeof(struct sockaddr_pppox));
    sp_info.sa_family = AF_PPPOX;
    sp_info.sa_protocol = PX_PROTO_OE;
    sp_info.sa_addr.pppoe.sid = sessId;   /* PPPoE session ID */
    memcpy(sp_info.sa_addr.pppoe.remote, dstMac, ETH_ALEN); /* Remote MAC address */
    memcpy(sp_info.sa_addr.pppoe.dev, devName, strlen(devName)); 

    return sp_info;
}

/**************************************************************************
** Function:    pptp_pppox_connect()
** Description: Actually connect to pppox kernel module with the structure
**                  got by pptp_pppox_get_info().
** Parameters:  (int *) poxfd -- pointer of file descriptor for pppox
**              (int *) pppfd -- pointer of file descriptor for ppp device
** Return:      (int)err -- Fail = -1
**                          Success = 0.
**************************************************************************/
/*  wklin modified start, 07/31/2007 */
int  pptp_pppox_connect(int *poxfd, int *pppfd)
{
    struct sockaddr_pppox lsp;
    int err = -1;
    int chindex;
	int flags;

    memset(&lsp, 0, sizeof(struct sockaddr_pppox));
    lsp = pptp_pppox_get_info();

    if (*poxfd >= 0) {
        /* Connect pptp kernel connection */
        err = connect(*poxfd, (struct sockaddr*)&lsp,
            sizeof(struct sockaddr_pppox));
        if (err == 0) {
            /* Get PPP channel */
            if (ioctl(*poxfd, PPPIOCGCHAN, &chindex) == -1) {
                fprintf(stderr, "Couldn't get channel number");
                return -1;
            }

            if (*pppfd >= 0) {
                /* Attach to PPP channel */
                if ((err = ioctl(*pppfd, PPPIOCATTCHAN, &chindex)) < 0) {
                    fprintf(stderr, "Couldn't attach to channel");
                    return -1;
                }
#ifdef MULTIPLE_PPPOE
                if (ioctl(*pppfd, PPPIOCCONNECT, &ppp_ifunit) < 0) {
                    fprintf(stderr, "Couldn't attach to PPP unit %d.\n", ppp_ifunit);
                    return -1;
                }
#endif
                flags = fcntl(*pppfd, F_GETFL);
                if (flags == -1 || fcntl(*pppfd, F_SETFL, flags | O_NONBLOCK) == -1) {
                    fprintf(stderr, "Couldn't set /dev/ppp (channel) to nonblock");
                    return -1;
                }
            }
            else {
                fprintf(stderr, "Couldn't reopen /dev/ppp");
                return -1;
            }
        }
        else {
            fprintf(stderr, "Couldn't connect pppox, err: %d, %s", err, strerror(errno));
            return -1;
        }
    }
    return 0;
}
/*  wklin modified end, 07/31/2007 */

/**************************************************************************
** Function:    pptp_pppox_release()
** Description: Release the connection between user program and pppox kernel
**                  driver with ioctl() and connect(), and clear the
**                  essential information in kernel.
** Parameters:  (int *) poxfd -- pointer of file descriptor for pppox
**              (int *) pppfd -- pointer of file descriptor for ppp device
** Return:      (int)err -- Fail = -1
**                          Success = 0.
**************************************************************************/
void pptp_pppox_release(int *poxfd, int *pppfd)
{
    struct sockaddr_pppox lsp;
    int err = -1;

    if (*poxfd >= 0) {
        memset(&lsp, 0, sizeof(struct sockaddr_pppox));
        lsp = pptp_pppox_get_info();
        if (*pppfd >= 0) {
            /* Detach from PPP */
    	    if (ioctl(*pppfd, PPPIOCDETACH) < 0)
                ; /* fprintf(stderr, "pptp_pppox_release ioctl(PPPIOCDETACH)
                     failed\n"); */ /*  wklin removed, 07/26/2007 */
	    }

        /* Release pptp kernel connection */
        lsp.sa_addr.pppoe.sid = 0;
        err = connect(*poxfd, (struct sockaddr*)&lsp,
            sizeof(struct sockaddr_pppox));
        if (err != 0)
            fprintf(stderr, "Couldn't connect to pptp kernel module\n");
    }
    else
        fprintf(stderr, "Couldn't connect socket to pppox\n");
}
/*  added end, Winster Chan, 06/26/2006 */

int
open_interface(char *if_name, unsigned short type, char *hw_addr)
{
    int optval = 1, rv;
    struct ifreq ifr;

    if ((rv = socket(PF_INET, SOCK_PACKET, htons(type))) < 0) {
        perror("pppoe: socket");
        return -1;
    }

    if (setsockopt(rv, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
        perror("pppoe: setsockopt");
        return -1;
    }

    if (hw_addr != NULL) {
        strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name));

        if (ioctl(rv, SIOCGIFHWADDR, &ifr) < 0) {
            perror("pppoe: ioctl(SIOCGIFHWADDR)");
            return -1;
        }

        if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) {
            fprintf(error_file, "pppoe: interface %s is not Ethernet!\n", if_name);
            return -1;
        }
        memcpy(hw_addr, ifr.ifr_hwaddr.sa_data, sizeof(ifr.ifr_hwaddr.sa_data));
    }
    return rv;
}

/*  added start Winster Chan 12/02/2005 */
int
get_padx_tag(struct pppoe_tag *pTag, int nLen, pPadxTag pTagStruc, const unsigned short tagType)
{
    int bExitLoop = 0, nSearchLen = 0, bTagFound = 0;
    struct pppoe_tag *pNextTag;

    pTagStruc->usPadLen = 0;
    pTagStruc->nPadLen = 0;
    pTagStruc->pPadStart = NULL;

    while (!bExitLoop && (nSearchLen < nLen)) {
        if (htons(pTag->type) == tagType) {
            pTagStruc->usPadLen = pTag->length;
            pTagStruc->nPadLen = (int)(htons(pTag->length));
            pTagStruc->pPadStart = (char *)pTag + TAG_STRUCT_SIZE;
            bExitLoop = 1;
            bTagFound = 1;
        }
        else {
            pNextTag = (struct pppoe_tag *)((char *)pTag +
                (TAG_STRUCT_SIZE + (int)(htons(pTag->length))));
            nSearchLen += (int)((TAG_STRUCT_SIZE + (int)(htons(pTag->length))));
            pTag = pNextTag;
        }
    }

    return bTagFound;
}
/*  added end Winster Chan 12/02/2005 */

int
create_padi(struct pppoe_packet *packet, const char *src, const char *name)
{
    int size;

    if (packet == NULL)
	return 0;

    /* printf("Winster: create_padi\n"); */

    size = sizeof(struct pppoe_packet) + sizeof(struct pppoe_tag);
    if (name != NULL)
	size += strlen(name);

#ifdef __linux__
    memcpy(packet->ethhdr.h_dest, MAC_BCAST_ADDR, 6);
    memcpy(packet->ethhdr.h_source, src, 6);
    packet->ethhdr.h_proto = htons(ETH_P_PPPOE_DISC);
#else
    memcpy(packet->ethhdr.ether_dhost, MAC_BCAST_ADDR, 6);
    memcpy(packet->ethhdr.ether_shost, src, 6);
    packet->ethhdr.ether_type = htons(ETH_P_PPPOE_DISC);
#endif
    packet->ver = 1;
    packet->type = 1;
    packet->code = CODE_PADI;
    packet->session = 0;
    packet->length = htons(size - sizeof(struct pppoe_packet));

    /* fill out a blank service-name tag */
    (*(struct pppoe_tag *)(packet+1)).type = htons(TAG_SERVICE_NAME);
    (*(struct pppoe_tag *)(packet+1)).length = name ? htons(strlen(name)) : 0;
    if (name != NULL)
	memcpy((char *)(packet + 1) + sizeof(struct pppoe_tag), name,
	       strlen(name));

    return size;
}

int
create_padr(struct pppoe_packet *packet, const char *src, const char *dst,
	    char *name)
{
    int size;
    /*  added start Winster Chan 11/25/2005 */
    char *pCookieStart = NULL;
    int nCookieSize = 0;
    int nNameSize = 0;
    unsigned short usCookieSize = 0;
    sPadxTag sTag;
    /*  added end Winster Chan 11/25/2005 */
    /*  add start, Max Ding, 09/22/2008 for @add TAG_RELAY_SESSION_ID */
    char *pRelaySessionIdStart = NULL;
    int nRelaySessionIdSize = 0;
    unsigned short usRelaySessionIdSize = 0;
    char *pPacketPoint = packet;
    /*  add end, Max Ding, 09/22/2008 */

    if (packet == NULL)
	return 0;

    size = sizeof(struct pppoe_packet) + TAG_STRUCT_SIZE;
    if (name != NULL) {
    	size += strlen(name);
        /*  added start Winster Chan 11/25/2005 */
        nNameSize = strlen(name);
        /*  added end Winster Chan 11/25/2005 */
    }

    /*  added start Winster Chan 12/02/2005 */
    /* Add length of AC cookie to packet size */
    if (get_padx_tag((struct pppoe_tag *)pado_tags, pado_tag_size, &sTag, TAG_AC_COOKIE)) {
        nCookieSize = sTag.nPadLen;
        usCookieSize = sTag.usPadLen;
        pCookieStart = sTag.pPadStart;
        size += (int)(TAG_STRUCT_SIZE + sTag.nPadLen);
    }
    /*  added end Winster Chan 12/02/2005 */
    /*  add start, Max Ding, 09/22/2008 for @add TAG_RELAY_SESSION_ID */
    if (get_padx_tag((struct pppoe_tag *)pado_tags, pado_tag_size, &sTag, TAG_RELAY_SESSION_ID)) {
        nRelaySessionIdSize = sTag.nPadLen;
        usRelaySessionIdSize = sTag.usPadLen;
        pRelaySessionIdStart = sTag.pPadStart;
        size += (int)(TAG_STRUCT_SIZE + sTag.nPadLen);
    }
    /*  add end, Max Ding, 09/22/2008 */

#ifdef __linux__
    memcpy(packet->ethhdr.h_dest, dst, 6);
    memcpy(packet->ethhdr.h_source, src, 6);
    packet->ethhdr.h_proto = htons(ETH_P_PPPOE_DISC);
#else
    memcpy(packet->ethhdr.ether_dhost, dst, 6);
    memcpy(packet->ethhdr.ether_shost, src, 6);
    packet->ethhdr.ether_type = htons(ETH_P_PPPOE_DISC);
#endif
    packet->ver = 1;
    packet->type = 1;
    packet->code = CODE_PADR;
    packet->session = 0;
    packet->length = htons(size - sizeof(struct pppoe_packet));

    /* fill out a blank service-name tag */
    (*(struct pppoe_tag *)(packet+1)).type = htons(TAG_SERVICE_NAME);
    (*(struct pppoe_tag *)(packet+1)).length = name ? htons(strlen(name)) : 0;
    if (name != NULL)
	memcpy((char *)(packet + 1) + TAG_STRUCT_SIZE, name,
	       strlen(name));

    pPacketPoint = (char *)(packet+1)+TAG_STRUCT_SIZE+nNameSize;/*  added by Max Ding, 09/23/2008 @add TAG_RELAY_SESSION_ID */
    /*  modified start, wklin, 03/27/2007, winster doesn't count name len */
    /*  added start Winster Chan 11/25/2005 */
    /* fill out the AC cookie tag from PADO */
    if (nCookieSize > 0) {
        (*(struct pppoe_tag *)((char *)(packet+1)+TAG_STRUCT_SIZE+
                               nNameSize)).type = htons(TAG_AC_COOKIE);
        (*(struct pppoe_tag *)((char *)(packet+1)+TAG_STRUCT_SIZE+
                               nNameSize)).length = usCookieSize;
        memcpy((char *)(packet+1)+(2*TAG_STRUCT_SIZE)+nNameSize,
            (char *)pCookieStart, nCookieSize);
        pPacketPoint = (char *)(packet+1)+(2*TAG_STRUCT_SIZE)+nNameSize+nCookieSize;/*  added by Max Ding, 09/23/2008 @add TAG_RELAY_SESSION_ID */
    }
    /*  added end Winster Chan 11/25/2005 */
    /*  modified end, wklin, 03/27/2007 */
    /*  add start, Max Ding, 09/22/2008 for @add TAG_RELAY_SESSION_ID */
    if (nRelaySessionIdSize > 0) {
        (*(struct pppoe_tag *)(pPacketPoint)).type = htons(TAG_RELAY_SESSION_ID);
        (*(struct pppoe_tag *)(pPacketPoint)).length = usRelaySessionIdSize;
        memcpy((pPacketPoint+TAG_STRUCT_SIZE),
            (char *)pRelaySessionIdStart, nRelaySessionIdSize);
        pPacketPoint += TAG_STRUCT_SIZE + nRelaySessionIdSize;
    }
    /*  add end, Max Ding, 09/22/2008 */

    memset(((char *)packet) + size, 0, 14);
    return size;
}

/*  added end Winster Chan 12/02/2005 */
int
create_padt(struct pppoe_packet *packet, const char *src, const char *dst, unsigned short nSessId)
{
    int size;
    char *pCookieStart = NULL;
    int nCookieSize = 0;
    unsigned short usCookieSize = 0;
    sPadxTag sTag;

    if (packet == NULL)
	return 0;

    size = sizeof(struct pppoe_packet) + TAG_STRUCT_SIZE;

    if (get_padx_tag((struct pppoe_tag *)pado_tags, pado_tag_size, &sTag, TAG_AC_COOKIE)) {
        nCookieSize = sTag.nPadLen;
        usCookieSize = sTag.usPadLen;
        pCookieStart = sTag.pPadStart;
        size += (int)(TAG_STRUCT_SIZE + sTag.nPadLen);
    }

#ifdef __linux__
    memcpy(packet->ethhdr.h_dest, dst, 6);
    memcpy(packet->ethhdr.h_source, src, 6);
    packet->ethhdr.h_proto = htons(ETH_P_PPPOE_DISC);
#else
    memcpy(packet->ethhdr.ether_dhost, dst, 6);
    memcpy(packet->ethhdr.ether_shost, src, 6);
    packet->ethhdr.ether_type = htons(ETH_P_PPPOE_DISC);
#endif
    packet->ver = 1;
    packet->type = 1;
    packet->code = CODE_PADT;
    /*packet->session = session;*/
    packet->session = nSessId;
    packet->length = htons(size - sizeof(struct pppoe_packet));

    /* fill out a blank generic-error tag */
    (*(struct pppoe_tag *)(packet+1)).type = htons(TAG_GENERIC_ERROR);
    (*(struct pppoe_tag *)(packet+1)).length = 0;

    /* fill out the AC cookie tag from PADO */
    if (nCookieSize > 0) {
        (*(struct pppoe_tag *)((char *)(packet+1)+TAG_STRUCT_SIZE)).type
            = htons(TAG_AC_COOKIE);
        (*(struct pppoe_tag *)((char *)(packet+1)+TAG_STRUCT_SIZE)).length
            = usCookieSize;
        memcpy((char *)(packet+1)+(2*TAG_STRUCT_SIZE),
            (char *)pCookieStart, nCookieSize);
    }

    memset(((char *)packet) + size, 0, 14);
    return size;
}
/*  added end Winster Chan 12/02/2005 */

/*  added start pling 09/09/2009 */
int create_lcp_terminate_request
(
    struct pppoe_packet *packet,
    const char *src, const char *dst,
    unsigned short sess_id
)
{
    int size;
    struct pppoe_packet *ppp_data_start;
    char ppp_data[] = 
    {
        0xc0, 0x21,     /* PPP Link Control Protocol */
        0x05,           /* Code: Terminate request */
        0x01,           /* Identifier */
        0x00, 0x10,     /* Length: 16 */
        0x55, 0x73, 0x65, 0x72, 0x20, /* "user " */
        0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74 /* "request" */
    };

    if (packet == NULL)
	    return 0;

    size = sizeof(struct pppoe_packet) + sizeof(ppp_data);

#ifdef __linux__
    memcpy(packet->ethhdr.h_dest, dst, 6);
    memcpy(packet->ethhdr.h_source, src, 6);
    packet->ethhdr.h_proto = htons(ETH_P_PPPOE_SESS);
#else
    memcpy(packet->ethhdr.ether_dhost, dst, 6);
    memcpy(packet->ethhdr.ether_shost, src, 6);
    packet->ethhdr.ether_type = htons(ETH_P_PPPOE_SESS);
#endif
    packet->ver = 1;    
    packet->type = 1;
    packet->code = CODE_SESS;
    packet->session = sess_id;
    packet->length = htons(size - sizeof(struct pppoe_packet));

    ppp_data_start = packet + 1;
    memcpy(ppp_data_start, ppp_data, sizeof(ppp_data));

    memset(((char *)packet) + size, 0, 14);
    return size;
}
/*  added end pling 09/09/2009 */

unsigned short fcstab[256] = {
    0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
    0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
    0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
    0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
    0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
    0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
    0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
    0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
    0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
    0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
    0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
    0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
    0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
    0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
    0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
    0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
    0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
    0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
    0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
    0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
    0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
    0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
    0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
    0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
    0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
    0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
    0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
    0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
    0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
    0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
    0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
    0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};

#define PPPINITFCS16    0xffff  /* Initial FCS value */
#define PPPGOODFCS16    0xf0b8  /* Good final FCS value */
/*
 * Calculate a new fcs given the current fcs and the new data.
 */
unsigned short pppfcs16(register unsigned short fcs,
			register unsigned char * cp,
			register int len)
{
/*    assert(sizeof (unsigned short) == 2);
    assert(((unsigned short) -1) > 0); */

    while (len--)
	fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];

    return (fcs);
}

#define FRAME_ESC 0x7d
#define FRAME_FLAG 0x7e
#define FRAME_ADDR 0xff
#define FRAME_CTL 0x03
#define FRAME_ENC 0x20

#define ADD_OUT(c) { *out++ = (c); n++; if (opt_verbose) fprintf(log_file, "%x ", (c)); }

void encode_ppp(int fd, unsigned char *buf, int len)
{
    static int first = 0;
    unsigned char out_buf[PACKETBUF];
    unsigned char *out = out_buf;
    unsigned char header[2], tail[2];
    int i,n;
    unsigned short fcs;
#ifndef MULTIPLE_PPPOE
    time_t tm;
#endif
    header[0] = FRAME_ADDR;
    header[1] = FRAME_CTL;
    fcs = pppfcs16(PPPINITFCS16, header, 2);
    fcs = pppfcs16(fcs, buf, len) ^ 0xffff;
    tail[0] = fcs & 0x00ff;
    tail[1] = (fcs >> 8) & 0x00ff;
#ifndef MULTIPLE_PPPOE
    if (opt_verbose)
    {
	time(&tm);
	fprintf(log_file, "%sWriting to pppd: \n", ctime(&tm));
    }
#endif
    n = 0;
    if (!first) {
	ADD_OUT(FRAME_FLAG);
	first = 1;
    }
    ADD_OUT(FRAME_ADDR); /* the header - which is constant */
    ADD_OUT(FRAME_ESC);
    ADD_OUT(FRAME_CTL ^ FRAME_ENC);

    for (i = 0; i < len; i++)
	if (buf[i] == FRAME_FLAG || buf[i] == FRAME_ESC || buf[i] < 0x20)
	{
	    ADD_OUT(FRAME_ESC);
	    ADD_OUT(buf[i] ^ FRAME_ENC);
	}
	else
	    ADD_OUT(buf[i]);

    for (i = 0; i < 2; i++) {
	if (tail[i] == FRAME_FLAG || tail[i] == FRAME_ESC || tail[i] < 0x20) {
	    ADD_OUT(FRAME_ESC);
	    ADD_OUT(tail[i] ^ FRAME_ENC);
	} else
	    ADD_OUT(tail[i]);
    }
    ADD_OUT(FRAME_FLAG);

    write(fd, out_buf, n);

#ifndef MULTIPLE_PPPOE
    if (opt_verbose)
	fprintf(log_file, "\n");
#endif
}

int
create_sess(struct pppoe_packet *packet, const char *src, const char *dst,
	    unsigned char *buf, int bufsize, int sess, int *bufRemain)
{
    int size;
    int i, o = 0;
    int nBufLen = 0;
    int nTotalLen = 0, nAllLen = 0;
    int nTemp = 0;
    unsigned short usTotalLen = 0x0;
    unsigned char bufLen[2];

    /* Clear the length of remain buffer */
    *bufRemain = 0;

    if (opt_fwd || !((buf[0] == FRAME_FLAG) || (buf[0] == FRAME_ADDR)))
    {
    	if (opt_fwd_search) /* search for a valid packet */
    	{
    	    while (*buf++ != FRAME_FLAG && bufsize != 0)
    		bufsize--;
            if (bufsize == 0) {
                fprintf(error_file, "create_sess: bufsize == 0\n");
                return 0;
            }
    	}
    	else
    	{
    	    /* fprintf(error_file, "create_sess: invalid data\n"); */ /*  wklin
                                                                         removed,
                                                                         07/26/2007
                                                                         */
    	    return 0;
    	}
    }

    bufLen[0] = 0x0;
    bufLen[1] = 0x0;
    if (bufsize <= 4) {
        *bufRemain = bufsize;
        return 0;
    }
    else {
        nTemp = (buf[0] == FRAME_FLAG) ? 4 : 3;
        for (i = nTemp; i < bufsize; i++) {
            if (buf[i] == FRAME_ESC) {
                if ((o == 4) || (o == 5))
                    bufLen[o-4] = buf[++i] ^ FRAME_ENC;
                else
                    ++i;
            }
            else {
                if ((o == 4) || (o == 5))
                    bufLen[o-4] = buf[i];
            }
            o++;

            /* Get the total length from IP packet header */
            if (o == 6) {
                usTotalLen = ntohs(*(unsigned short *)bufLen);
                nTotalLen = (int)usTotalLen;
                nAllLen = nTotalLen + 5;
            }

            /* Get the total buffer length of packet */
            if ((o >= 6) && (o == nAllLen)) {
                nBufLen = i + 1;
                break;
            }
        }

        if ((o < nAllLen) || (o < 6)) {
            *bufRemain = bufsize;
            return 0;
        }
        o = 0;
    }

    /* for (i = (buf[0] == FRAME_FLAG ? 4 : 3); i < bufsize; i++) { */
    for (i = nTemp; i < bufsize; i++) {
        if (buf[i] == FRAME_ESC) {
            buf[o++] = buf[++i] ^ FRAME_ENC;
        }
        else {
            buf[o++] = buf[i];
        }

        if (o == nAllLen)
            break;
    }   /* End for() */

    /* Compute the length of remain buffer */
    *bufRemain = bufsize - nBufLen;

    bufsize = nTotalLen + 2;

    if (packet == NULL) {
        return 0;
    }

    size = sizeof(struct pppoe_packet) + bufsize;

#ifdef __linux__
    memcpy(packet->ethhdr.h_dest, dst, 6);
    memcpy(packet->ethhdr.h_source, src, 6);
    packet->ethhdr.h_proto = htons(ETH_P_PPPOE_SESS);
#else
    memcpy(packet->ethhdr.ether_dhost, dst, 6);
    memcpy(packet->ethhdr.ether_shost, src, 6);
    packet->ethhdr.ether_type = htons(ETH_P_PPPOE_SESS);
#endif
    packet->ver = 1;
    packet->type = 1;
    packet->code = CODE_SESS;
    packet->session = sess;
    packet->length = htons(size - sizeof(struct pppoe_packet));

    return size;
}

int
send_packet(int sock, struct pppoe_packet *packet, int len, const char *ifn)
{
    struct sockaddr addr;
    int c;
#ifndef MULTIPLE_PPPOE
    time_t tm;
#endif
    memset(&addr, 0, sizeof(addr));
    strcpy(addr.sa_data, ifn);

#ifndef MULTIPLE_PPPOE
    if (opt_verbose == 1)
    {
	time(&tm);
	fprintf(log_file, "%sSending ", ctime(&tm));
	print_packet(packet);
	fputc('\n', log_file);
    }
#endif

    if ((c = sendto(sock, packet, len, 0, &addr, sizeof(addr))) < 0) {
	/* fprintf(error_file, "send_packet c[%d] = sendto(len = %d)\n", c, len); */
	perror("pppoe: sendto (send_packet)");
    }

    return c;
}
#ifdef MULTIPLE_PPPOE
int
read_packet_nowait(int sock, struct pppoe_packet *packet, int *len)
{
    socklen_t fromlen = PACKETBUF;

    if (recvfrom(sock, packet, PACKETBUF, 0,
             NULL /*(struct sockaddr *)&from*/, &fromlen) < 0) {
        perror("pppoe: recv (read_packet_nowait)");
        return -1;
    }

    return sock;
}

int
read_packet(int sock, struct pppoe_packet *packet, int *len)
{
    socklen_t fromlen = PACKETBUF;
    fd_set fdset;
    struct timeval tma;

    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    tma.tv_usec = 0; 
    tma.tv_sec = 3; /* wait for 3 seconds at most */
    while(1) {
        if (select(sock + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL,
		    &tma) <= 0) {
            return -1; /* timeout or error */
        } else if (FD_ISSET(sock, &fdset)) {
            if (recvfrom(sock, packet, PACKETBUF, 0,
                  NULL /*(struct sockaddr *)&from*/, &fromlen) < 0) {
            	perror("pppoe: recv (read_packet)");
                return -1;
            } else if (memcmp(packet->ethhdr.h_dest,src_addr,sizeof(src_addr))!=0){
		/* fprintf(stderr, "pppoe: received a packet not for me.\n");*/
		continue;
	    }
        }
        return sock;
    }
}
#endif
#ifndef MULTIPLE_PPPOE
/*  wklin added start, 08/10/2007 */
int
read_packet2(int sock, struct pppoe_packet *packet, int *len)
{
#if defined(__GNU_LIBRARY__) && __GNU_LIBRARY__ < 6
    int fromlen = PACKETBUF;
#else
    socklen_t fromlen = PACKETBUF;
#endif
    time_t tm;

    if (recvfrom(sock, packet, PACKETBUF, 0,
                 NULL /*(struct sockaddr *)&from*/, &fromlen) < 0) {
        perror("pppoe: recv (read_packet2)");
        return -1;
    }

    if (opt_verbose)
    {
        time(&tm);
        fprintf(log_file, "Received packet at %s", ctime(&tm));
        print_packet(packet);
        fputc('\n', log_file);
    }

     return sock;
}
/*  wklin added end, 08/10/2007 */

int
read_packet(int sock, struct pppoe_packet *packet, int *len)
{
/*    struct sockaddr_in from; */
#if defined(__GNU_LIBRARY__) && __GNU_LIBRARY__ < 6
    int fromlen = PACKETBUF;
#else
    socklen_t fromlen = PACKETBUF;
#endif
    time_t tm;

    time(&tm);

    while(1) {
	/* wklin modified start, 01/10/2007 */
        fd_set fdset;
        struct timeval tm;
	
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
    	tm.tv_usec = 0; 
	tm.tv_sec = 3; /* wait for 3 seconds */
	if (select(sock + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) <= 0) {
            return -1; /* timeout or error */
	} else if (FD_ISSET(sock, &fdset)) {
	    if (recvfrom(sock, packet, PACKETBUF, 0,
		 NULL /*(struct sockaddr *)&from*/, &fromlen) < 0) {
	        perror("pppoe: recv (read_packet)");
	        return -1;
	    }
	}
	/* wklin modified end, 01/10/2007 */
	if (opt_verbose)
	{
	    fprintf(log_file, "Received packet at %s", ctime(&tm));
	    print_packet(packet);
	    fputc('\n', log_file);
	}

	return sock;
    }
}
#endif

void sigchild(int src) {
    clean_child = 1;
}

void cleanup_and_exit(int status) {
#ifdef MULTIPLE_PPPOE
    if (pppfd > 0)
        close(pppfd); 
    if (poxfd > 0)
        close(poxfd);
    if (disc_sock > 0)
        close(disc_sock);
    if (sess_sock > 0)
        close(sess_sock);
    close(1);

    exit(status);
#else
    /*  modified start, Winster Chan, 06/26/2006 */
#ifdef NEW_WANDETECT
    if (!bWanDetect) /*  added by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#endif
    {
        pptp_pppox_release(&poxfd, &pppfd);
        close(pppfd); pppfd = -1;
        close(poxfd); poxfd = -1;
    }
    /*  modified end, Winster Chan, 06/26/2006 */

    close(disc_sock);
#ifdef NEW_WANDETECT
    if (!bWanDetect) /*  added by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#endif
        close(sess_sock);
    close(1);

    exit(status);
#endif
}

void sigint(int src)
{
#ifdef MULTIPLE_PPPOE
    struct pppoe_packet *packet = NULL;
    int pkt_size;
    FILE *fp;

    /*  modified start pling 10/08/2009 */
    /* Don't reference pppd_listen any more as this var is not used. */
    /* if (disc_sock >= 0 && (pppd_listen >= 0)) { */
    if (disc_sock > 0) {
    /*  modified end pling 10/08/2009 */
        /* allocate packet once */
        packet = malloc(PACKETBUF);
        assert(packet != NULL);

        /* send PADT */
        if ((pkt_size = create_padt(packet, src_addr, dst_addr, session)) == 0) {
              fprintf(stderr, "pppoe: unable to create PADT packet\n");
              if (packet != NULL) 
                  free(packet);
              exit(1);
        }
        if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
            fprintf(stderr, "pppoe: unable to send PADT packet\n");
            if (packet != NULL) 
                free(packet);
            exit(1);
        }  else {
            ; /* fprintf(stderr, "PPPOE: PADT sent*\n"); */
        }
    }

    if (ppp_ifunit == 0)
        fp = fopen(PPP_PPPOE_SESSION, "w+");
    else
        fp = fopen(PPP_PPPOE2_SESSION, "w+");

    if (fp) {
        /* Clear the PPPoE server MAC address and Session ID */
        fprintf(fp, "%02x:%02x:%02x:%02x:%02x:%02x %d\n",
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0);
        fclose(fp);
    }

    if (packet != NULL) 
        free(packet);
    cleanup_and_exit(1);
#else
    /*  added start Winster Chan 12/02/2005 */
    struct pppoe_packet *packet = NULL;
    int pkt_size;
    FILE *fp;
    time_t tm;

    /*  modified start pling 10/08/2009 */
    /* Don't reference pppd_listen any more as this var is not used. */
    /* if (disc_sock && (pppd_listen > 0)) { */
    if (disc_sock > 0) {
    /*  modified end pling 10/08/2009 */
        /* allocate packet once */
        packet = malloc(PACKETBUF);
        assert(packet != NULL);

        /* send PADT */
        if ((pkt_size = create_padt(packet, src_addr, dst_addr, session)) == 0) {
  	        fprintf(stderr, "pppoe: unable to create PADT packet\n");
            /* exit(1); */
        }
        if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
            fprintf(stderr, "pppoe: unable to send PADT packet\n");
            /* exit(1); */
        }  else {
            time(&tm);
            fprintf(stderr, "PPPOE: PADT sent* %s\n",ctime(&tm)); /*  wklin added, 07/26/2007 */
        }
    }
    /*  added end Winster Chan 12/02/2005 */

    /*  added start Winster Chan 12/05/2005 */
    if (!(fp = fopen(PPP_PPPOE_SESSION, "w"))) {
        perror(PPP_PPPOE_SESSION);
    }
    else {
        /* Clear the PPPoE server MAC address and Session ID */
        fprintf(fp, "%02x:%02x:%02x:%02x:%02x:%02x %d\n",
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0);
        fclose(fp);
    }

    if (packet != NULL) 
        free(packet);

    /*  added end Winster Chan 12/05/2005 */
    cleanup_and_exit(1);
#endif
}

/* added start James 11/12/2008 @new_internet_detection*/
#ifdef NEW_WANDETECT
void sigint2(int src)
{
    struct pppoe_packet *packet = NULL;
    int pkt_size;
    FILE *fp;
    time_t tm;

    /* allocate packet once */
    packet = malloc(PACKETBUF);
    assert(packet != NULL);

    /*  added start pling 09/09/2009 */
    /* Send LCP terminate request first, before PADT,
     *  to make the PPP server terminates our session.
     */
    if ((pkt_size = create_lcp_terminate_request(packet, 
                        src_addr, dst_addr, session)) == 0) {
        fprintf(stderr, "pppoe: unable to create LCP terminate req packet\n");
    } else {
        sleep(1);
        if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
            fprintf(stderr, "pppoe: unable to send PADT packet\n");
        } else {
            time(&tm);
            fprintf(stderr, "PPPOE: LCP Terminate Req sent* %s\n",ctime(&tm));
            sleep(1);
        }
    }
    /*  added end pling 09/09/2009 */

    /* send PADT */
    if ((pkt_size = create_padt(packet, src_addr, dst_addr, session)) == 0) {
            fprintf(stderr, "pppoe: unable to create PADT packet\n");
        /* exit(1); */
    }
    if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
        fprintf(stderr, "pppoe: unable to send PADT packet\n");
        /* exit(1); */
    }  else {
        time(&tm);
        fprintf(stderr, "PPPOE: PADT sent* %s\n",ctime(&tm)); /*  wklin added, 07/26/2007 */
    }

    /*  added end Winster Chan 12/02/2005 */

    /*  added start Winster Chan 12/05/2005 */
    if (!(fp = fopen(PPP_PPPOE_SESSION, "w"))) {
        perror(PPP_PPPOE_SESSION);
    }
    else {
        /* Clear the PPPoE server MAC address and Session ID */
        fprintf(fp, "%02x:%02x:%02x:%02x:%02x:%02x %d\n",
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0);
        fclose(fp);
    }
    if (packet != NULL) 
        free(packet);

    /*  added end Winster Chan 12/05/2005 */
    cleanup_and_exit(1);
    
}
#endif
/*  added end James 11/12/2008 @new_internet_detection */ 

void sess_handler(void) {
    /* pull packets of sess_sock and feed to pppd */
    static struct pppoe_packet *packet = NULL;
    int pkt_size;

#ifdef BUGGY_AC
/* the following code deals with buggy AC software which sometimes sends
   duplicate packets */
#define DUP_COUNT 10
#define DUP_LENGTH 20
    static unsigned char dup_check[DUP_COUNT][DUP_LENGTH];
    int i, ptr = 0;
#endif /* BUGGY_AC */

    if (!packet) {
#ifdef BUGGY_AC
        memset(dup_check, 0, sizeof(dup_check));
#endif
        /* allocate packet once */
        packet = malloc(PACKETBUF);
        assert(packet != NULL);
    }

    /* while(1) */
    {
#ifdef MULTIPLE_PPPOE
        if (read_packet_nowait(sess_sock,packet,&pkt_size) != sess_sock)
#else
	    if (read_packet2(sess_sock,packet,&pkt_size) != sess_sock)
#endif
                return;
#ifdef __linux__
	    if (memcmp(packet->ethhdr.h_source, dst_addr, sizeof(dst_addr)) != 0)
#else
	    if (memcmp(packet->ethhdr.ether_shost, dst_addr, sizeof(dst_addr)) != 0)
#endif
	        return; /* packet not from AC */
#ifdef MULTIPLE_PPPOE
        if (memcmp(packet->ethhdr.h_dest, src_addr, sizeof(src_addr)) != 0) {
	    /* fprintf(stderr, "pppoe: received a session packet not for
	     * me.\n"); */
            return; 
		}
#endif        
	    if (packet->session != session)
	        return; /* discard other sessions */
#ifdef __linux__
	    if (packet->ethhdr.h_proto != htons(ETH_P_PPPOE_SESS))
	    {
	        fprintf(log_file, "pppoe: invalid session proto %x detected\n",
		        ntohs(packet->ethhdr.h_proto));
	        return;
	    }
#else
	    if (packet->ethhdr.ether_type != htons(ETH_P_PPPOE_SESS))
	    {
	        fprintf(log_file, "pppoe: invalid session proto %x detected\n",
		        ntohs(packet->ethhdr.ether_type));
                return;
	    }
#endif
	    if (packet->code != CODE_SESS) {
	        fprintf(log_file, "pppoe: invalid session code %x\n", packet->code);
	        return;
	    }
#if BUGGY_AC
    	/* we need to go through a list of recently-received packets to
	       make sure the AC hasn't sent us a duplicate */
	    for (i = 0; i < DUP_COUNT; i++)
	        if (memcmp(packet, dup_check[i], sizeof(dup_check[0])) == 0)
		        return; /* we've received a dup packet */
#define min(a,b) ((a) < (b) ? (a) : (b))
	    memcpy(dup_check[ptr], packet, min(ntohs(packet->length),
						    sizeof(dup_check[0])));
	    ptr = ++ptr % DUP_COUNT;
#endif /* BUGGY_AC */

	    encode_ppp(1, (unsigned char *)(packet+1), ntohs(packet->length));
    }
}

void pppd_handler(void) {
  /* take packets from pppd and feed them to sess_sock */
  struct pppoe_packet *packet = NULL;
#ifndef MULTIPLE_PPPOE
  time_t tm;
#endif
  static sPktBuf pktBuf[BUFRING]; /*  wklin modified, use static */
  int nPkt = 0;
  /* unsigned char buf[PACKETBUF]; */
  int len, pkt_size;
  int i, bufRemain = 0, bufPos;
  unsigned char *currBufStart;
  static int first_in = 1; /*  wklin added, 08/10/2007 */

  if (first_in) {
      first_in = 0;
      /* Clear data buffer */
      for (i=0; i<BUFRING; i++) {
           memset(pktBuf[i].packetBuf, 0x0, sizeof(pktBuf[i].packetBuf));
      }
  }

  {
    /* Read in data buffer, Maximum size is 4095 bytes for evey one read() */
    if ((len = read(0, &(pktBuf[nPkt].packetBuf[(20+bufRemain)]), (4095-bufRemain))) < 0) {
      perror("pppoe");
      fprintf(error_file, "pppd_handler: read packet error len < 0\n");
      /* exit(1); */
      return;
    }
    if (len == 0) {
      /*  wklin modified start, 07/27/2007 */
      /* fprintf(error_file, "pppd_handler: read packet len = 0 bytes\n"); */
      /* usleep(10000);*/ /* sleep 10ms */
      /*  wklin modified end, 07/27/2007 */
      /* continue; */
        return;
    }
    /* Append the length of previous remained data */
    len += bufRemain;
#ifndef MULTIPLE_PPPOE
    if (opt_verbose == 1) {
        time(&tm);
        fprintf(log_file, "\n%sInput of %d bytes:\n", ctime(&tm), len);
        /*print_hex(buf, len);*/
        print_hex(pktBuf[nPkt].packetBuf, len);
        fputc('\n', log_file);
    }
#endif
    bufPos = 0;
    packet = &(pktBuf[nPkt].packetBuf[0]);
    currBufStart = &(pktBuf[nPkt].packetBuf[20]);
    do {
        /* Segment and compose packet from buffer */
        if ((pkt_size = create_sess(packet, src_addr, dst_addr, currBufStart, len,
                                    session, &bufRemain)) == 0) {
          /* Copy incomplete content to next buffer */
          if (bufRemain > 0) {
            if ((nPkt+1) < BUFRING) {
                memcpy(&(pktBuf[nPkt+1].packetBuf[20]),
                       &(pktBuf[nPkt].packetBuf[(4115-bufRemain)]),         /* 4115 = 4095 + 20 */
                       bufRemain);
            }
            else {
                memcpy(&(pktBuf[0].packetBuf[20]),
                       &(pktBuf[(BUFRING-1)].packetBuf[(4115-bufRemain)]),  /* 4115 = 4095 + 20 */
                       bufRemain);
            }
          }
          break;
        }

        len -= bufRemain;

        /* Send the completely composed packet */
        if (send_packet(sess_sock, packet, pkt_size, if_name) < 0) {
          fprintf(error_file, "pppd_handler: unable to send PPPoE packet\n");
          /* exit(1); */
          return;
        }

        /* Check if all the contents of buffer were processed */
        if (bufRemain <= 0) {
            break;
        }
        else {
            /* Renew the positions of packet header and data buffer */
            bufPos += len;
            currBufStart = &(pktBuf[nPkt].packetBuf[bufPos+20]);
            packet = &(pktBuf[nPkt].packetBuf[bufPos]);
            len = bufRemain;
        }
    } while (bufRemain > 0);

    /* Process next read buffer */
    nPkt++;
    if (nPkt >= BUFRING)
        nPkt = 0;

  }
}
#ifdef MULTIPLE_PPPOE
/*  Bob Guo added start 10/25/2007*/
#define SERVER_RECORD_FILE	"/tmp/ppp/PPPoE_server_record"
#define UPTIME_FILE			"/proc/uptime"

static int checkServerRecord(unsigned char *pServerMac)
{
	FILE *fp=NULL;
	char line[64];
	char macAddr[32];
	char *token;
	int iTimeStamp = 0;

	fp = fopen(SERVER_RECORD_FILE, "r");
	if(fp == NULL)
	{
		goto _exit;
	}

	sprintf(macAddr, "%02x:%02x:%02x:%02x:%02x:%02x", *pServerMac, *(pServerMac+1), *(pServerMac+2), 
													*(pServerMac+3), *(pServerMac+4), *(pServerMac+5));

	while (fgets(line, sizeof(line), fp)) 
	{
		if( strncmp(line, macAddr, strlen(macAddr)) == 0)
		{
            /* the mac address exist in the list */
			token = strtok(line, " ");
			if(token == NULL)
				goto _exit;
			token = strtok(NULL, " ");
			if(token)
			{
				iTimeStamp = atoi(token);
			}
			break;
		}
	}

_exit:

	if(fp)
		fclose(fp);

	return iTimeStamp;
}

static void updateServerRecord(unsigned char *pServerMac)
{
    FILE *fp=NULL, *fp_uptime=NULL;
	char line[64];
	char macAddr[32];
	char uptime[32];
    char *token;
    int bFound;
	
	fp = fopen(SERVER_RECORD_FILE, "r+");
	if(fp == NULL)
	{
		fp = fopen(SERVER_RECORD_FILE, "w+");
		if(fp == NULL)
			goto _exit;
	}

	sprintf(macAddr, "%02x:%02x:%02x:%02x:%02x:%02x", *pServerMac, *(pServerMac+1), *(pServerMac+2), 
													*(pServerMac+3), *(pServerMac+4), *(pServerMac+5));
													
	fp_uptime = fopen(UPTIME_FILE, "r");
	if(fp_uptime == NULL)
		goto _exit;

	if(fgets(line, sizeof(line), fp_uptime))
	{
		token = strtok(line, " .\t\n");
		if(token == NULL)
			goto _exit;
        
        int i = atoi(token);
        sprintf(uptime, "%010d", i);
	}

	bFound = 0;

	while (fgets(line, sizeof(line), fp)) 
	{
		if( strncmp(line, macAddr, strlen(macAddr)) == 0)
		{
            /* the mac address exist in the list */
			bFound = 1;
			break;
		}
	}
	
	if(bFound)
	{
		int ioffset = 0 - (strlen(line));
		fseek(fp, ioffset, SEEK_CUR);
	}
	else
	{
		fseek(fp, 0, SEEK_END);
	}
	fprintf(fp, "%s %s\n", macAddr, uptime);

_exit:
	if(fp)
		fclose(fp);
	if(fp_uptime)
		fclose(fp_uptime);

}

/*  Bob Guo added end 10/25/2007 */
#endif
int main(int argc, char **argv)
{
    struct pppoe_packet *packet = NULL;
#ifdef MULTIPLE_PPPOE
    /*  Bob Guo added start 10/25/2007*/    
    struct pppoe_packet *packet2 = NULL;
    struct pppoe_packet *packet_temp = NULL;
    int pkt2_size;
    int iOldestTimeStamp, iTimeStamp;
    /*  Bob Guo added end 10/25/2007*/
#endif
    int pkt_size;
    /*  added start Winster Chan 12/05/2005 */
    FILE *fp;
    char buf[64];
    /*  added end Winster Chan 12/05/2005 */
    int fd; /* wklin added, 07/26/2007 */

    int opt;
    int ret_sock; /*  wklin added, 12/27/2007 */

    /*  wklin added start, 08/10/2007 */
    fd_set allfdset;
#ifndef MULTIPLE_PPPOE
    struct timeval alltm;
#endif
    /*  wklin added end, 08/10/2007 */
    time_t tm;

#ifdef NEW_WANDETECT
    bWanDetect = 0;/*  added by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#endif

    /* initialize error_file here to avoid glibc2.1 issues */
     error_file = stderr;

    /*  wklin added start, 07/26/2007 */
    fd = open("/dev/console", O_WRONLY);
    if (fd != -1)
        dup2(fd, 2);
    /*  wklin added end, 07/26/2007 */

    /* parse options */
    /*  wklin modified, 03/27/2007, add service name option S */
#ifdef MULTIPLE_PPPOE
    /* while ((opt = getopt(argc, argv, "I:L:VE:F:S:P:")) != -1) */
    while ((opt = getopt(argc, argv, "I:L:VE:F:S:R:P:")) != -1)/*  modified by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#else
    /* while ((opt = getopt(argc, argv, "I:L:VE:F:S:")) != -1) */
    while ((opt = getopt(argc, argv, "I:L:VE:F:S:R:")) != -1)/*  modified by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#endif
	switch(opt)
	{
	case 'F': /* sets invalid forwarding */
	    if (*optarg == 'a') /* always forward */
		opt_fwd = 1;
	    else if (*optarg == 's') /* search for flag */
		opt_fwd_search = 1;
	    else
		fprintf(stderr, "Invalid forward option %c\n", *optarg);
	    break;

	case 'I': /* sets interface */
	    if (if_name != NULL)
		free(if_name);
        /*  wklin modified, 03/27/2007 */
	    if ((if_name=malloc(strlen(optarg)+1)) == NULL)
	    {
		fprintf(stderr, "malloc\n");
		exit(1);
	    }
	    strcpy(if_name, optarg);
#ifndef MULTIPLE_PPPOE
            /* wklin added start, 01/15/2007 */
            if (log_file != NULL)
                fclose(log_file);
            if ((log_file=fopen("/dev/null", "w")) == NULL) {
                fprintf(stderr, "fopen\n");
                exit(1);
            }
            /* wklin added end, 01/15/2007 */
#endif
	    break;

	case 'L': /* log file */
	    opt_verbose = 1;
	    if (log_file != NULL)
		fclose(log_file);
	    if ((log_file=fopen(optarg, "w")) == NULL)
	    {
		fprintf(stderr, "fopen\n");
		exit(1);
	    }
	    if (setvbuf(log_file, NULL, _IONBF, 0) != 0)
	    {
		fprintf(stderr, "setvbuf\n");
		exit(1);
	    }
	    break;
	case 'V': /* version */
	    printf("pppoe version %d.%d\n", VERSION_MAJOR, VERSION_MINOR);
	    exit(0);
	    break;
	case 'E': /* error file */
	    if ((error_file = fopen(optarg, "w")) == NULL)
	    {
		fprintf(stderr, "fopen\n");
		exit(1);
	    }
	    if (setvbuf(error_file, NULL, _IONBF, 0) != 0)
	    {
		fprintf(stderr, "setvbuf\n");
		exit(1);
	    }
	    break;
        /*  wklin added start, 03/27/2007, service name */
	case 'S': /* set service name*/
	    if (service_name != NULL)
		    free(service_name);
	    if ((service_name=malloc(strlen(optarg)+1)) == NULL) {
#ifdef MULTIPLE_PPPOE
            fprintf(stderr, "pppoe: malloc error.\n");
#else
		    fprintf(stderr, "malloc\n");
#endif
		    exit(1);
	    }
	    strcpy(service_name, optarg);
	    break;
        /*  wklin added end, 03/27/2007 */
#ifdef MULTIPLE_PPPOE
        /*  wklin added start, 08/16/2007, service name */
    case 'P': /* ppp ifunit */
        ppp_ifunit = 1;
        break;
        /*  wklin added end, 08/16/2007 */
#endif
        /*  add start, Max Ding, 04/23/2009 not use pppd to reduce memory usage */
        case 'R': /* wan detect purpose */
#ifdef NEW_WANDETECT
            bWanDetect = 1;
#endif
            break;
        /*  add end, Max Ding, 04/23/2009 */
	default:
	    fprintf(stderr, "Unknown option %c\n", optopt);
	    exit(1);
	}

    /* allocate packet once */
    packet = malloc(PACKETBUF);
    assert(packet != NULL);

    /* create the raw socket we need */

    signal(SIGINT, sigint);
    signal(SIGTERM, sigint);
#ifdef NEW_WANDETECT
    signal(SIGUSR1, sigint2);/* added James 11/11/2008 @new_internet_detection*/
#endif

#ifndef MULTIPLE_PPPOE
#ifdef NEW_WANDETECT
    if (!bWanDetect) /*  added by Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#endif
    {
        /*  added start, Winster Chan, 06/26/2006 */
        pptp_pppox_open(&poxfd, &pppfd);
        /*  added end, Winster Chan, 06/26/2006 */
    }
#endif

    if ((disc_sock = open_interface(if_name,ETH_P_PPPOE_DISC,src_addr)) < 0)
    {
		fprintf(error_file, "pppoe: unable to create raw socket\n");
#ifdef MULTIPLE_PPPOE
        exit(1);
#else
		return 1;
#endif
    }
#ifdef MULTIPLE_PPPOE
    /* initiate connection */
    if (ppp_ifunit == 0)
        fp = fopen(PPP_PPPOE_SESSION, "r"); 
    else /* ifunit == 1 */
	fp = fopen(PPP_PPPOE2_SESSION, "r");

    if (fp) {
#else
    /* initiate connection */

    /*  added start Winster Chan 12/05/2005 */
    if (!(fp = fopen(PPP_PPPOE_SESSION, "r"))) {
        ; /* perror(PPP_PPPOE_SESSION); */ /*  wklin removed, 08/13/2007 */
    }
    else {
#endif
        unsigned int nMacAddr[ETH_ALEN];
        int nSessId, i;
        char cMacAddr[ETH_ALEN];
        unsigned short usSessId;

        /* Init variables */
        for (i=0; i<ETH_ALEN; i++) {
            nMacAddr[i] = 0;
            cMacAddr[i] = 0x0;
        }
        nSessId = 0;

        /* Get the PPPoE server MAC address and Session ID from file */
        if(fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%02x:%02x:%02x:%02x:%02x:%02x %d",
                &nMacAddr[0], &nMacAddr[1], &nMacAddr[2],
                &nMacAddr[3], &nMacAddr[4], &nMacAddr[5],
                &nSessId);

            /* Transfer types of data */
            for (i=0; i<ETH_ALEN; i++) {
                cMacAddr[i] = (char)nMacAddr[i];
            }
            usSessId = (unsigned short)nSessId;

            /* Check if the previous MAC address and Session ID exist. */
            if (!((cMacAddr[0]==0x0) && (cMacAddr[1]==0x0) && (cMacAddr[2]==0x0) &&
                 (cMacAddr[3]==0x0) && (cMacAddr[4]==0x0) && (cMacAddr[5]==0x0)) &&
                (htons(usSessId) != 0x0)) {
                    
                /* send PADT to server to clear the previous connection */
                if ((pkt_size = create_padt(packet, src_addr, cMacAddr,
                                    (unsigned short)htons(usSessId))) == 0) {
          	        fprintf(stderr, "pppoe: unable to create PADT packet\n");
                    fclose(fp);
                    /*exit(1);*/
#ifdef MULTIPLE_PPPOE
                    exit(1);
#else
                    cleanup_and_exit(1); /*  modified by EricHuang, 05/24/2007 */
#endif
                }
                if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
                    fprintf(stderr, "pppoe: unable to send PADT packet\n");
                    fclose(fp);
#ifdef MULTIPLE_PPPOE
                    exit(1);
#else
                    /*exit(1);*/
                    cleanup_and_exit(1); /*  modified by EricHuang, 05/24/2007 */
#endif
                } else {
#ifdef MULTIPLE_PPPOE
                    ; /* fprintf(stderr, "PPPOE: PADT sent\n"); */
#else
                    time(&tm);
                    fprintf(stderr, "PPPOE: PADT sent %s\n",ctime(&tm)); /*  wklin added, 07/26/2007 */
#endif
                }

                /*  modified start, Winster Chan, 06/26/2006 */
                /*pptp_pppox_release(&poxfd, &pppfd);*/ /*  removed by EricHuang, 05/24/2007 */
                /*  modified end, Winster Chan, 06/26/2006 */

                /* Waiting 3 seconds for server finishing the termination */
                sleep(3);
            }
        }
        fclose(fp);
    }
    /*  added end Winster Chan 12/05/2005 */
#ifdef MULTIPLE_PPPOE
    memset(dst_addr, 0, sizeof(dst_addr));

resend_padi:
#endif
    /* start the PPPoE session */
    /*  wklin modified, 03/27/2007, add service name */
    if ((pkt_size = create_padi(packet, src_addr, service_name)) == 0) {
	fprintf(stderr, "pppoe: unable to create PADI packet\n");
	exit(1);
    }
    /* send the PADI packet */
    if (send_packet(disc_sock, packet, pkt_size, if_name) < 0) {
	fprintf(stderr, "pppoe: unable to send PADI packet\n");
	exit(1);
    }

    time(&tm); /*  wklin added, 12/27/2007 */
    /* wait for PADO */
    while ((ret_sock = read_packet(disc_sock, packet, &pkt_size)) != disc_sock ||
	   (packet->code != CODE_PADO )) { /*  wklin modified, 12/27/2007 */
#ifndef MULTIPLE_PPPOE
	fprintf(log_file, "pppoe: unexpected packet %x\n",
		packet->code);
#endif
	/*  wklin added start, 12/27/2007 */
	if (ret_sock == disc_sock &&
		(packet->code == CODE_PADI || packet->code == CODE_PADR)) {
	    if (time(NULL) - tm < 3)
		continue;

	}
	/*  wklin added end, 12/27/2007 */
	/* wklin added start, 01/10/2007 */
    	/* fprintf(stderr, "pppoe: resend PADI\n"); */
    	/* start the PPPoE session */
        /*  wklin modified, 03/27/2007, add service name */
    	if ((pkt_size = create_padi(packet, src_addr, service_name)) == 0) {
		fprintf(stderr, "pppoe: unable to create PADI packet\n");
		exit(1);
    	}
    	/* send the PADI packet */
    	if (send_packet(disc_sock, packet, pkt_size, if_name) < 0) {
		fprintf(stderr, "pppoe: unable to send PADI packet\n");
		exit(1);
    	}
	/* wklin added end, 01/10/2007 */
	time(&tm); /*  wklin added, 12/27/2007 */
#ifndef MULTIPLE_PPPOE
	continue;
#endif
    }
#ifdef MULTIPLE_PPPOE
    /*  Bob Guo added start 10/25/2007 */
    
    iOldestTimeStamp = checkServerRecord(packet->ethhdr.h_source);
    if(iOldestTimeStamp)
    {
        packet2 = malloc(PACKETBUF);
        if(packet2)
        {
            while( read_packet(disc_sock, packet2, &pkt2_size) == disc_sock )
            {
                if(packet2->code == CODE_PADO)
                {
                    iTimeStamp = checkServerRecord(packet2->ethhdr.h_source);
                    if(iTimeStamp < iOldestTimeStamp)
                    {
                        packet_temp = packet;
                        packet = packet2;
                        packet2 = packet_temp;
                        pkt_size = pkt2_size;
                    }
                }
            }
            free(packet2);
        }
    }
    /*  Bob Guo added end 10/25/2007 */
#endif
    /*  added start pling 12/20/2006 */
    /* Touch a file on /tmp to indicate PADO is received */
    if (1)
    {
        FILE *fp;
        struct sysinfo info;
#ifdef MULTIPLE_PPPOE
        /* fprintf(stderr, "PPPOE%d: PADO received\n", ppp_ifunit); */
#else
        /*  wklin modified start, 07/26/2007 */
        /* system("echo \"DEBUG: PADO received\" > /dev/console"); */
        time(&tm);
        fprintf(stderr, "PPPOE: PADO received %s\n", ctime(&tm));
        /*  wklin modified end, 07/26/2007 */
#endif
        if ((fp = fopen("/tmp/PADO", "w")) != NULL) {
            sysinfo(&info);  /* save current time in file */
            fprintf(fp, "%ld", info.uptime);
            fclose(fp);
        }
    }
    /*  added end ling 12/20/2006 */


#ifdef __linux__
    memcpy(dst_addr, packet->ethhdr.h_source, sizeof(dst_addr));
#else
    memcpy(dst_addr, packet->ethhdr.ether_shost, sizeof(dst_addr));
#endif
    /*  added start Winster Chan 11/25/2005 */
    /* Stored tags of PADO */
    memset(pado_tags, 0x0, sizeof(pado_tags));
    if ((htons(packet->length) < 0) || (htons(packet->length) > sizeof(pado_tags))) {
        pado_tag_size = sizeof(pado_tags);
    }
    else {
        pado_tag_size = (int)(htons(packet->length));
    }
    memcpy((char *)pado_tags, (char *)(packet + 1), pado_tag_size);
    /*  added end Winster Chan 11/25/2005 */

    /* send PADR */
    /*  wklin modified, 03/27/2007, add service name */
    if ((pkt_size = create_padr(packet, src_addr, dst_addr, service_name)) == 0) {
	fprintf(stderr, "pppoe: unable to create PADR packet\n");
	exit(1);
    }
    if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
	fprintf(stderr, "pppoe: unable to send PADR packet\n");
	exit(1);
    }

    /*  wklin modified start, 07/31/2008 */
    /* 0. Process only packets from our target pppoe server.
     * 1. track the time when sending PADR.
     * 2. If received PADO, resend PADR (needed?).
     * 3. If retried for 5 times, send PADT.
     * 4. If wait for more than 10 seconds after sending the previous PADR, 
     *    send PADT.
     */
    time(&tm);
    /* wait for PADS */
#ifdef __linux__
    while ((ret_sock=read_packet(disc_sock, packet, &pkt_size)) != disc_sock ||
            (memcmp(packet->ethhdr.h_source, dst_addr, sizeof(dst_addr)) != 0) || 
            (packet->code != CODE_PADS && packet->code != CODE_PADT))
#else
    while (read_packet(disc_sock, packet, &pkt_size) != disc_sock ||
	   (memcmp(packet->ethhdr.ether_shost,
		   dst_addr, sizeof(dst_addr)) != 0))
#endif
    {
        static int retried=0; /* wklin added, 01/26/2007 */

        if (ret_sock != disc_sock) {
            if (time(NULL)-tm > 10) {
                retried = 0;
                packet->code = CODE_PADT;  /* fake packet */
                break;
            }
            else
                continue;
        }
        /* Resend PADR only if it's from our target pppoe server and 
         * the code is PADO, otherwise ignore the received packet.
         */
        if (memcmp(packet->ethhdr.h_source, dst_addr, sizeof(dst_addr)) == 0) {
            if (packet->code != CODE_PADO) { /* PADI? PADR? */
                retried = 0;
                packet->code = CODE_PADT; /* fake packet */
                break;
            }

            /* Received PADO */
            if (retried++ == 5) {
                retried = 0;
                packet->code = CODE_PADT; 
                break;
            }

            /*  add start James 04/10/2009 */
            /*if we recieve two PADO(from the same MAC) in 2 seconds, we only send one PADR.*/
            if (time(NULL) - tm < 2)
                continue;
            /*  add end James 04/10/2009 */

            /* send PADR */
            /*  wklin modified, 03/27/2007, add service name */
            if ((pkt_size = create_padr(packet, src_addr, dst_addr, service_name)) == 0) {
	        fprintf(stderr, "pppoe: unable to create PADR packet\n");
                exit(1);
            }
            if (send_packet(disc_sock, packet, pkt_size+14, if_name) < 0) {
	        fprintf(stderr, "pppoe: unable to send PADR packet\n");
                exit(1);
            }
            time(&tm); /* track the time when sending PADR */
        }
        /* else.. packets not from target server */
#ifndef MULTIPLE_PPPOE
        /* wklin modified end, 01/26/2007 */        
        continue;
#endif
    } /* end while waiting PADS */
    /*  wklin modified end, 07/31/2008 */

    /*  James added start, 11/12/2008 @new_internet_detection */
#ifdef NEW_WANDETECT
    if (bWanDetect && packet->code == CODE_PADS)
    {
        FILE *fp;
        struct sysinfo info;

        time(&tm);
        fprintf(stderr, "PPPOE: PADS received %s\n", ctime(&tm));
        if ((fp = fopen("/tmp/PADS", "w")) != NULL) {
            sysinfo(&info);  /* save current time in file */
            fprintf(fp, "%ld", info.uptime);
            fclose(fp);
        }
    }
#endif
    /*  James added end, 11/12/2008 @new_internet_detection */

    if (packet->code == CODE_PADT) /* early termination */
    {
#ifdef MULTIPLE_PPPOE
        sleep(3);
        goto resend_padi;
#else
	    cleanup_and_exit(0);
#endif
    }

    session = packet->session;

    /*  wklin added start, 07/31/2007 */
    if (session == 0) { /* PADS generic error */
        sleep(3); /* wait for 3 seconds and exit, retry */
#ifdef MULTIPLE_PPPOE
		goto resend_padi;
#else
	    cleanup_and_exit(0);
#endif
    }

    /*  add start, Max Ding, 04/23/2009 not use pppd to reduce memory usage */
#ifdef NEW_WANDETECT
    if (bWanDetect) 
    {
        /* Create PADT to end it */
        sigint2(SIGUSR1);
        /*cleanup_and_exit(0);*/
    }
#endif
    /*  add end, Max Ding, 04/23/2009 */

    time(&tm);
#ifdef MULTIPLE_PPPOE
    fprintf(stderr, "PPPOE%d: PADS received (%d) %s\n", 
            ppp_ifunit, ntohs(session), ctime(&tm));
    /*  Bob Guo added start 10/25/2007*/            
    updateServerRecord(packet->ethhdr.h_source);
    /*  Bob Guo added end 10/25/2007*/
#else
    fprintf(stderr, "PPPOE: session id = %d, %s\n", htons(session), ctime(&tm));
#endif

    /*  wklin added end, 07/31/2007 */

    if ((sess_sock = open_interface(if_name,ETH_P_PPPOE_SESS,NULL)) < 0) {
    	fprintf(log_file, "pppoe: unable to create raw socket\n");
#ifdef MULTIPLE_PPPOE
        exit(1);
#else
    	cleanup_and_exit(1);
#endif
    }

    /*  added start, Winster Chan, 06/26/2006 */
    memcpy(dstMac, dst_addr, ETH_ALEN);
    sessId = packet->session;

    /* Connect pptp kernel module */

#ifdef MULTIPLE_PPPOE
    pptp_pppox_open(&poxfd, &pppfd);

    if (poxfd == -1) {
        fprintf(stderr, "pppoe: poxfd == -1\n");
        cleanup_and_exit(1);
        /*
	    close(poxfd);
	    exit(1);*/
    }

    if (pppfd == -1) {
        fprintf(stderr, "pppoe: pppfd == -1\n");
        cleanup_and_exit(1);
        /*
	    close(pppfd);
	    exit(1);*/
    }

    if ( 0 > pptp_pppox_connect(&poxfd, &pppfd)) {
        fprintf(stderr, "error connect to pppox\n");
        cleanup_and_exit(1);
        
	
	/*
	exit(1);*/
    }

    if (ppp_ifunit == 0)
        fp = fopen(PPP_PPPOE_SESSION, "w+");
    else
        fp = fopen(PPP_PPPOE2_SESSION, "w+");

    if (fp) {
        /* Save the PPPoE server MAC address and Session ID */
        fprintf(fp, "%02x:%02x:%02x:%02x:%02x:%02x %d\n",
            (unsigned char)(dst_addr[0]), (unsigned char)(dst_addr[1]),
            (unsigned char)(dst_addr[2]), (unsigned char)(dst_addr[3]),
            (unsigned char)(dst_addr[4]), (unsigned char)(dst_addr[5]),
            (int)htons(session));
        fclose(fp);
    }
#else

    /*  wklin modified start, 07/31/2007 */
    if ( 0 > pptp_pppox_connect(&poxfd, &pppfd))
        cleanup_and_exit(1);
    /*  wklin modified end, 07/31/2007 */
    /*  added end, Winster Chan, 06/26/2006 */

    /*  added start Winster Chan 12/05/2005 */
    if (!(fp = fopen(PPP_PPPOE_SESSION, "w"))) {
        perror(PPP_PPPOE_SESSION);
    }
    else {
        /* Save the PPPoE server MAC address and Session ID */
        fprintf(fp, "%02x:%02x:%02x:%02x:%02x:%02x %d\n",
            (unsigned char)(dst_addr[0]), (unsigned char)(dst_addr[1]),
            (unsigned char)(dst_addr[2]), (unsigned char)(dst_addr[3]),
            (unsigned char)(dst_addr[4]), (unsigned char)(dst_addr[5]),
            (int)htons(session));
        fclose(fp);
    }
    /*  added end Winster Chan 12/05/2005 */
#endif
    clean_child = 0;
    signal(SIGCHLD, sigchild);

    while (1) {
	    FD_ZERO(&allfdset);
	    FD_SET(sess_sock, &allfdset);
	    FD_SET(disc_sock, &allfdset);
	    FD_SET(0, &allfdset);
	    if (select((sess_sock>disc_sock?sess_sock:disc_sock) + 1, &allfdset, 
                    (fd_set *) NULL, (fd_set *) NULL, NULL) <= 0)
            continue; /* timeout or error */

        if (FD_ISSET(disc_sock, &allfdset)) {
#ifdef MULTIPLE_PPPOE
            if (read_packet_nowait(disc_sock, packet, &pkt_size) == disc_sock) {
		        if (memcmp(packet->ethhdr.h_dest, src_addr, sizeof(src_addr))!=0){
		            /* fprintf(stderr, "pppoe: received a packet not for
		            * me.\n");*/
		            continue;
		        }
                if (packet->code == CODE_PADT && packet->session == session) {
                    time(&tm);
                    fprintf(stderr, "PPPOE%d: PADT received (%d) %s\n", ppp_ifunit, ntohs(session), ctime(&tm));
                    break; /* go terminate */
		        }
            }   
#else
            if ((read_packet2(disc_sock, packet, &pkt_size) == disc_sock) &&
	       (memcmp(packet->ethhdr.h_source, dst_addr, sizeof(dst_addr))==0)){
                if (packet->code == CODE_PADT) {
                    time(&tm);
                    fprintf(stderr, "PPPOE: PADT received (%d/%d), %s\n", 
                        ntohs(packet->session), ntohs(session),ctime(&tm));
                } 
                if (packet->code == CODE_PADT && packet->session == session)
                    break; /* cleanup and exit */
            }   
#endif
        }

        if (FD_ISSET(sess_sock, &allfdset)) {
            sess_handler();
        }

        if (FD_ISSET(0, &allfdset)) {
            pppd_handler();
        }
    }   
    cleanup_and_exit(0); /*  wklin added, 08/10/2007 */
    return 0;
}


