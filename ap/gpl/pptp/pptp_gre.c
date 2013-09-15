/* pptp_gre.c  -- encapsulate PPP in PPTP-GRE.
 *                Handle the IP Protocol 47 portion of PPTP.
 *                C. Scott Ananian <cananian@alumni.princeton.edu>
 *
 * $Id: pptp_gre.c,v 1.39 2005/07/11 03:23:48 quozl Exp $
 */

#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "ppp_fcs.h"
#include "pptp_msg.h"
#include "pptp_gre.h"
#include "util.h"
#include "pqueue.h"
/*  added start, Winster Chan, 06/26/2006 */
#include "pptpox.h"
#include <stdio.h>
#include <linux/types.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/ioctl.h>
/*  added end, Winster Chan, 06/26/2006 */

#define PACKET_MAX 8196
/* test for a 32 bit counter overflow */
#define WRAPPED( curseq, lastseq) \
    ((((curseq) & 0xffffff00) == 0) && \
     (((lastseq) & 0xffffff00 ) == 0xffffff00))

static u_int32_t ack_sent, ack_recv;
static u_int32_t seq_sent, seq_recv;
static u_int16_t pptp_gre_call_id, pptp_gre_peer_call_id;
gre_stats_t stats;

/*  added start, Winster Chan, 06/26/2006 */
static int pox_fd = -1;
/*  added start, Winster Chan, 06/26/2006 */

typedef int (*callback_t)(int cl, void *pack, unsigned int len);

/* decaps_hdlc gets all the packets possible with ONE blocking read */
/* returns <0 if read() call fails */
int decaps_hdlc(int fd, callback_t callback, int cl);
int encaps_hdlc(int fd, void *pack, unsigned int len);
int decaps_gre (int fd, callback_t callback, int cl);
int encaps_gre (int fd, void *pack, unsigned int len);
int dequeue_gre(callback_t callback, int cl);

#ifdef CODE_IN_USE  //Winster Chan added 05/16/2006
#include <stdio.h>
void print_packet(int fd, void *pack, unsigned int len)
{
    unsigned char *b = (unsigned char *)pack;
    unsigned int i,j;
    FILE *out = fdopen(fd, "w");
    fprintf(out,"-- begin packet (%u) --\n", len);
    for (i = 0; i < len; i += 16) {
        for (j = 0; j < 8; j++)
            if (i + 2 * j + 1 < len)
                fprintf(out, "%02x%02x ", 
                        (unsigned int) b[i + 2 * j],
                        (unsigned int) b[i + 2 * j + 1]);
            else if (i + 2 * j < len)
                fprintf(out, "%02x ", (unsigned int) b[i + 2 * j]);
        fprintf(out, "\n");
    }
    fprintf(out, "-- end packet --\n");
    fflush(out);
}
#endif  //CODE_IN_USE Winster Chan added 05/16/2006

/*** time_now_usecs ***********************************************************/
uint64_t time_now_usecs()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000000) + tv.tv_usec;
}

/*  added start, Winster Chan, 06/26/2006 */
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

/**************************************************************************
** Function:    pptp_pppox_open()
** Description: Open socket to kernel pppox driver, and open ppp device
** Parameters:  (int *) poxfd -- pointer of file descriptor for pppox
**              (int *) pppfd -- pointer of file descriptor for ppp device
** Return:      none.
**************************************************************************/
void pptp_pppox_open(int *poxfd, int *pppfd)
{
    /* Open socket to pppox kernel module */
    *poxfd = socket(AF_PPPOX,SOCK_STREAM,PX_PROTO_TP);
    if (*poxfd >= 0) {
        /*
         * Save the file descriptor for getting the GRE sequence number
         * from kernel side.
         */
        pox_fd = *poxfd;
        /* Open ppp device */
        *pppfd = open("/dev/ppp", O_RDWR);
    }
    else {
        *poxfd = -1;
        *pppfd = -1;
    }
}

/**************************************************************************
** Function:    pptp_pppox_get_info()
** Description: Get the essential information for connecting pptp kernel
**                  module. Such as Source IP, Destination IP, Remote MAC
**                  address, Device name, call_id, and peer_call_id, etc.
** Parameters:  none.
** Return:      (struct sockaddr_pptpox)sp_info -- structure of information.
**************************************************************************/
struct sockaddr_pptpox pptp_pppox_get_info(void)
{
    struct sockaddr_pptpox sp_info;
    FILE *fp = NULL;
    int nulluserip = 0;
    unsigned char buf[128];
    unsigned char userIp[IPV4_LEN], servIp[IPV4_LEN], arpIp[IPV4_LEN];
    unsigned char userNetMask[IPV4_LEN];    /* pling added 03/22/2012 */
    unsigned char dhcpIp[IPV4_LEN], gateWay[IPV4_LEN], netMask[IPV4_LEN];
    //unsigned char addrname[12];
    unsigned char addrname[32];
    unsigned int getIp[IPV4_LEN];
    int call_id = 0, peer_call_id = 0;
    unsigned char wan_ifname[IFNAMSIZ]; /* , added by EricHuang, 03/20/2007 */
    unsigned char pptp_gw[IPV4_LEN]; /* for static IP case,  added by EricHuang, 04/03/2007 */

    memset(&sp_info, 0, sizeof(struct sockaddr_pptpox));

    sp_info.sa_family = AF_PPPOX;
    sp_info.sa_protocol = PX_PROTO_TP;

    memset(userIp, 0, IPV4_LEN);
    memset(servIp, 0, IPV4_LEN);
    memset(arpIp, 0, IPV4_LEN);
    memset(dhcpIp, 0, IPV4_LEN);
    memset(gateWay, 0, IPV4_LEN);
    memset(netMask, 0, IPV4_LEN);

    /* Get user IP and server IP from /tmp/ppp/pptpIp file */
    if ((fp = fopen("/tmp/ppp/pptpIp", "r")) != NULL) {
        char user_nvram[] = "pptp_user_ip";
        char gw_nvram[] = "pptp_gateway_ip"; /*  added by EricHuang, 04/03/2007 */
        char user_netmask[] = "pptp_user_netmask"; /*  added by EricHuang, 04/03/2007 */

        /* Get WAN interface name */
        fgets(buf, sizeof(buf), fp);
        sscanf(buf, "%s", &wan_ifname[0]);
        memcpy(sp_info.sa_addr.pptp.dev, wan_ifname, IFNAMSIZ);

        while (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%s %d.%d.%d.%d",
                &addrname[0], &getIp[0], &getIp[1], &getIp[2], &getIp[3]);
            /* Get user IP */
            if (memcmp(addrname, user_nvram, sizeof(user_nvram)) == 0) {
                /* User IP is not NULL. It's tatic IP */
                addr_itox(userIp, getIp, IPV4_LEN);
                if ((userIp[0] == 0x0) && (userIp[1] == 0x0) &&
                    (userIp[2] == 0x0) && (userIp[3] == 0x0)) {
                    nulluserip = 1;
                }
                else {
                    memcpy(&sp_info.sa_addr.pptp.srcaddr, userIp, IPV4_LEN);
                    nulluserip = 0;
                }
            }
            /*  added start by EricHuang, 04/03/2007 */
            else if (memcmp(addrname, gw_nvram, sizeof(gw_nvram)) == 0) {
                addr_itox(pptp_gw, getIp, IPV4_LEN);
                //memcpy(&sp_info.sa_addr.pptp.srcaddr, pptp_gw, IPV4_LEN);
            }
            /*  added end by EricHuang, 04/03/2007 */
            /*  added start pling 03/22/2012 */
            /* Read the user's static netmask settings */
            else if (memcmp(addrname, user_netmask, sizeof(user_netmask)) == 0) {
                addr_itox(userNetMask, getIp, IPV4_LEN);
            }
            /*  added end pling 03/22/2012 */

        } /* End while() */
        fclose(fp);
        //unlink("/tmp/ppp/pptpIp");
    } /* End if(fp) */

    /*  added start by EricHuang, 03/20/2007 */
    /* get pptp server ip after gethostbyname, it called in pptp.c */
    if ((fp = fopen("/tmp/ppp/pptpSrvIp", "r")) != NULL) {
        memcpy(sp_info.sa_addr.pptp.dev, wan_ifname, IFNAMSIZ);

        while (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%d.%d.%d.%d",
                &getIp[0], &getIp[1], &getIp[2], &getIp[3]);

                addr_itox(servIp, getIp, IPV4_LEN);
                memcpy(&sp_info.sa_addr.pptp.dstaddr, servIp, IPV4_LEN);
        }
        fclose(fp);
    }
    /*  added end by EricHuang, 03/20/2007 */


    if (nulluserip) {
        /* NULL user IP, get user IP by dhcp client */
        char dhcp_ip[] = "user_ip_addr";
        char gate_way[] = "gateway_addr";
        char net_mask[] = "netmask_addr";

        if ((fp = fopen("/tmp/ppp/dhcpIp", "r")) != NULL) {
            /* Get IP by dhcp client */
            while (fgets(buf, sizeof(buf), fp)) {
                sscanf(buf, "%s %d.%d.%d.%d", &addrname[0],
                    &getIp[0], &getIp[1], &getIp[2], &getIp[3]);
                if (memcmp(addrname, dhcp_ip, sizeof(dhcp_ip)) == 0) {
                    /* Save dhcp user IP address */
                    addr_itox(dhcpIp, getIp, IPV4_LEN);
                    memcpy(&sp_info.sa_addr.pptp.srcaddr, dhcpIp, IPV4_LEN);
                }
                else if (memcmp(addrname, gate_way, sizeof(gate_way)) == 0) {
                    /* Save gateway address */
                    addr_itox(gateWay, getIp, IPV4_LEN);
                }
                else if (memcmp(addrname, net_mask, sizeof(net_mask)) == 0) {
                    /* Save net mask address */
                    addr_itox(netMask, getIp, IPV4_LEN);
                }
            } /* End while() */
            fclose(fp);
            //unlink("/tmp/ppp/dhcpIp");
        } /* End if(fp) */
    }

    /* Get MAC address and interface name from /proc/net/arp file */
    if ((fp = fopen("/proc/net/arp", "r")) != NULL) {
        int nMac[ETH_ALEN];
        unsigned char cMac[ETH_ALEN], ifname[IFNAMSIZ];
        unsigned char tmpChr[16];
        unsigned char cmpIp[IPV4_LEN];

        memset(nMac, 0, ETH_ALEN);
        memset(cMac, 0, ETH_ALEN);

        if (nulluserip) {
            /* Case: get IP by dhcp client */
            if (((dhcpIp[0] & netMask[0]) == (servIp[0] & netMask[0])) &&
                ((dhcpIp[1] & netMask[1]) == (servIp[1] & netMask[1])) &&
                ((dhcpIp[2] & netMask[2]) == (servIp[2] & netMask[2])) &&
                ((dhcpIp[3] & netMask[3]) == (servIp[3] & netMask[3]))) {
                /*
                 * dhcp IP & server IP are in same subnet,
                 * match the arp table with server IP.
                 */
                memcpy(cmpIp, servIp, IPV4_LEN);
            }
            else {
                /*
                 * dhcp IP & server IP are in different subnet,
                 * match the arp table with gateway IP.
                 */
                memcpy(cmpIp, gateWay, IPV4_LEN);
            }
        }
        else {
            /* Case: static IP */
            /* TODO: not consider gateway case currently! */
            
            /*  modified start by EricHuang, 04/03/2007 */
            /* pptp_gw will be solved in /rc/pptp.c, and if user don't enter the
               gateway address in the gui, we use pptp server ip as gateway ip.
            */
            //memcpy(cmpIp, servIp, IPV4_LEN);
            //memcpy(cmpIp, pptp_gw, IPV4_LEN); /* pling removed 03/22/2012 */
            /*  modified end by EricHuang, 04/03/2007 */
      
            /*  added start pling 03/22/2012 */
            /* Set the gateway properly according to subnet */
            if (((userIp[0] & userNetMask[0]) == (servIp[0] & userNetMask[0])) &&
                ((userIp[1] & userNetMask[1]) == (servIp[1] & userNetMask[1])) &&
                ((userIp[2] & userNetMask[2]) == (servIp[2] & userNetMask[2])) &&
                ((userIp[3] & userNetMask[3]) == (servIp[3] & userNetMask[3]))) {
                /*
                 * dhcp IP & server IP are in same subnet,
                 * match the arp table with server IP.
                 */
                memcpy(cmpIp, servIp, IPV4_LEN);
            }
            else {
                /*
                 * dhcp IP & server IP are in different subnet,
                 * match the arp table with gateway IP.
                 */
                memcpy(cmpIp, pptp_gw, IPV4_LEN);
            }
            /*  added end pling 03/22/2012 */
        }

        /* Skip the title name line */
        fgets(buf, sizeof(buf), fp);
        while (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%d.%d.%d.%d %s %s %02x:%02x:%02x:%02x:%02x:%02x %s %s",
                &getIp[0], &getIp[1], &getIp[2], &getIp[3],
                &tmpChr[0], &tmpChr[0],
                &nMac[0], &nMac[1], &nMac[2], &nMac[3], &nMac[4], &nMac[5],
                &tmpChr[0], &ifname[0]);
            addr_itox(arpIp, getIp, IPV4_LEN);
            /* Destination entry was found in the arp table */
            if (memcmp(cmpIp, arpIp, IPV4_LEN) == 0) {
                addr_itox(sp_info.sa_addr.pptp.remote, nMac, ETH_ALEN);
                break; /* Break off the while loop */
            }
        } /* End while() */
        fclose(fp);
    } /* End if(fp) */

    /* Get PPTP call_id & peer_call_id */
    if ((fp = fopen("/tmp/ppp/callIds", "r")) != NULL) {
        while (fgets(buf, sizeof(buf), fp)) {
            sscanf(buf, "%d %d", &call_id, &peer_call_id);

            sp_info.sa_addr.pptp.cid = call_id;
            sp_info.sa_addr.pptp.pcid = peer_call_id;
        }
        fclose(fp);
    } /* End if(fp) */

    return (struct sockaddr_pptpox)sp_info;
}

/**************************************************************************
** Function:    pptp_pppox_connect()
** Description: Actually connect to pppox kernel module with the structure
**                  got by pptp_pppox_get_info().
** Parameters:  (int *) poxfd -- pointer of file descriptor for pppox
**              (int *) pppfd -- pointer of file descriptor for ppp device
**              (u_int16_t) call_id -- local host call id
**              (u_int16_t) peer_call_id -- peer server call id
** Return:      (int)err -- Fail = -1
**                          Success = 0.
**************************************************************************/
void pptp_pppox_connect(int *poxfd, int *pppfd,
        u_int16_t call_id, u_int16_t peer_call_id)
{
    struct sockaddr_pptpox lsp;
    int err = -1;
    int chindex;
	int flags;

    memset(&lsp, 0, sizeof(struct sockaddr_pptpox));
    lsp = pptp_pppox_get_info();

    if (*poxfd >= 0) {
        /* Connect pptp kernel connection */
        err = connect(*poxfd, (struct sockaddr*)&lsp,
            sizeof(struct sockaddr_pptpox));
        if (err == 0) {
            /* Get PPP channel */
            if (ioctl(*poxfd, PPPIOCGCHAN, &chindex) == -1)
                warn("Couldn't get channel number");

            if (*pppfd >= 0) {
                /* Attach to PPP channel */
                if ((err = ioctl(*pppfd, PPPIOCATTCHAN, &chindex)) < 0)
                    warn("Couldn't attach to channel");
                flags = fcntl(*pppfd, F_GETFL);
                if (flags == -1 || fcntl(*pppfd, F_SETFL, flags | O_NONBLOCK) == -1)
                    warn("Couldn't set /dev/ppp (channel) to nonblock");
            }
            else
                warn("Couldn't reopen /dev/ppp");
        }
        else
            warn("Couldn't connect pppox, err: %d, %s", err, strerror(errno));
    }
}

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
    struct sockaddr_pptpox lsp;
    int err = -1;

    if (*poxfd >= 0) {
        memset(&lsp, 0, sizeof(struct sockaddr_pptpox));
        lsp = pptp_pppox_get_info();
        if (*pppfd >= 0) {
            /* Detach from PPP */
    	    if (ioctl(*pppfd, PPPIOCDETACH) < 0)
                warn("pptp_pppox_release ioctl(PPPIOCDETACH) failed");
	    }

        /* Release pptp kernel connection */
        lsp.sa_addr.pptp.srcaddr = 0;
        err = connect(*poxfd, (struct sockaddr*)&lsp,
            sizeof(struct sockaddr_pptpox));
        if (err != 0)
            warn("Couldn't connect to pptp kernel module");
    }
    else
        warn("Couldn't connect socket to pppox");
}
/*  added end, Winster Chan, 06/26/2006 */

/*** Open IP protocol socket **************************************************/
int pptp_gre_bind(struct in_addr inetaddr)
{
    struct sockaddr_in src_addr, loc_addr;
    extern struct in_addr localbind;
    int s = socket(AF_INET, SOCK_RAW, PPTP_PROTO);
    if (s < 0) { warn("socket: %s", strerror(errno)); return -1; }
    if (localbind.s_addr != INADDR_NONE) {
        bzero(&loc_addr, sizeof(loc_addr));
        loc_addr.sin_family = AF_INET;
        loc_addr.sin_addr   = localbind;
        if (bind(s, (struct sockaddr *) &loc_addr, sizeof(loc_addr)) != 0) {
            warn("bind: %s", strerror(errno)); close(s); return -1;
        }
    }
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr   = inetaddr;
    src_addr.sin_port   = 0;
    if (connect(s, (struct sockaddr *) &src_addr, sizeof(src_addr)) < 0) {
        warn("connect: %s", strerror(errno)); close(s); return -1;
    }
    return s;
}

/*** pptp_gre_copy ************************************************************/
void pptp_gre_copy(u_int16_t call_id, u_int16_t peer_call_id, 
		   int pty_fd, int gre_fd)
{
    int max_fd;
    pptp_gre_call_id = call_id;
    pptp_gre_peer_call_id = peer_call_id;
    /* Pseudo-terminal already open. */
    ack_sent = ack_recv = seq_sent = seq_recv = 0;
    /* weird select semantics */
    max_fd = gre_fd;
    if (pty_fd > max_fd) max_fd = pty_fd;
    /* Dispatch loop */
    for (;;) { /* until error happens on gre_fd or pty_fd */
        struct timeval tv = {0, 0};
        struct timeval *tvp;
        fd_set rfds;
        int retval;
        pqueue_t *head;
        int block_usecs = -1; /* wait forever */
        extern void connect_pppunit(void); /*  wklin added, 04/08/2011 */
        connect_pppunit(); /*  wklin added, 04/08/2011 */
        /* watch terminal and socket for input */
        FD_ZERO(&rfds);
        FD_SET(gre_fd, &rfds);
        FD_SET(pty_fd, &rfds);
        /*
         * if there are multiple pending ACKs, then do a minimal timeout;
         * else if there is a single pending ACK then timeout after 0,5 seconds;
         * else block until data is available.
         */
        if (ack_sent != seq_recv) {
            if (ack_sent + 1 == seq_recv)  /* u_int wrap-around safe */
                block_usecs = 500000;
            else
                /* don't use zero, this will force a resceduling */
                /* when calling select(), giving pppd a chance to */
                /* run. */
                block_usecs = 1;
        }
        /* otherwise block_usecs == -1, which means wait forever */
        /*
         * If there is a packet in the queue, then don't wait longer than
         * the time remaining until it expires.
         */
        head = pqueue_head();
        if (head != NULL) {
            int expiry_time = pqueue_expiry_time(head);
            if (block_usecs == -1 || expiry_time < block_usecs)
                block_usecs = expiry_time;
        }
        if (block_usecs == -1) {
            tvp = NULL;
        } else {
            tvp = &tv;
            tv.tv_usec = block_usecs;
            tv.tv_sec  = tv.tv_usec / 1000000;
            tv.tv_usec %= 1000000;
        }
        retval = select(max_fd + 1, &rfds, NULL, NULL, tvp);
        if (FD_ISSET(pty_fd, &rfds)) {
            if (decaps_hdlc(pty_fd, encaps_gre,  gre_fd) < 0)
                break;
        } else if (retval == 0 && ack_sent != seq_recv) {
            /* if outstanding ack */
            /* send ack with no payload */
            encaps_gre(gre_fd, NULL, 0);         
        }
        if (FD_ISSET(gre_fd, &rfds)) {
            if (decaps_gre (gre_fd, encaps_hdlc, pty_fd) < 0)
                break;
        }
        if (dequeue_gre (encaps_hdlc, pty_fd) < 0)
            break;
    }
    /* Close up when done. */
    close(gre_fd);
    close(pty_fd);
}

#define HDLC_FLAG         0x7E
#define HDLC_ESCAPE       0x7D
#define HDLC_TRANSPARENCY 0x20

/* ONE blocking read per call; dispatches all packets possible */
/* returns 0 on success, or <0 on read failure                 */
int decaps_hdlc(int fd, int (*cb)(int cl, void *pack, unsigned int len), int cl)
{
    unsigned char buffer[PACKET_MAX];
    unsigned int start = 0;
    int end;
    int status;
    static unsigned int len = 0, escape = 0;
    static unsigned char copy[PACKET_MAX];
    static int checkedsync = 0;
    /* start is start of packet.  end is end of buffer data */
    /*  this is the only blocking read we will allow */
    if ((end = read (fd, buffer, sizeof(buffer))) <= 0) {
        int saved_errno = errno;
        warn("short read (%d): %s", end, strerror(saved_errno));
	switch (saved_errno) {
	  case EMSGSIZE: {
	    int optval, optlen = sizeof(optval);
	    warn("transmitted GRE packet triggered an ICMP destination unreachable, fragmentation needed, or exceeds the MTU of the network interface");
#define IP_MTU 14
	    if(getsockopt(fd, IPPROTO_IP, IP_MTU, &optval, &optlen) < 0)
	      warn("getsockopt: %s", strerror(errno));
	    warn("getsockopt: IP_MTU: %d\n", optval);
	    return 0;
	  }
	case EIO:
    	    warn("pppd may have shutdown, see pppd log");
	    break;
	}
        return -1;
    }
    /* warn if the sync options of ppp and pptp don't match */
    if( !checkedsync) {
        checkedsync = 1;
        if( buffer[0] == HDLC_FLAG){
            if( syncppp )
                warn( "pptp --sync option is active, "
                        "yet the ppp mode is asynchronous!\n");
        }
	else if( !syncppp )
            warn( "The ppp mode is synchronous, "
                    "yet no pptp --sync option is specified!\n");
    }
    /* in synchronous mode there are no hdlc control characters nor checksum
     * bytes. Find end of packet with the length information in the PPP packet
     */
    if ( syncppp ){
        while ( start + 8 < end) {
            len = ntoh16(*(short int *)(buffer + start + 6)) + 4;
            /* note: the buffer may contain an incomplete packet at the end
             * this packet will be read again at the next read() */
            if ( start + len <= end)
                if ((status = cb (cl, buffer + start, len)) < 0)
                    return status; /* error-check */
            start += len;
        }
        return 0;
    }
    /* asynchronous mode */
    while (start < end) {
        /* Copy to 'copy' and un-escape as we go. */
        while (buffer[start] != HDLC_FLAG) {
            if ((escape == 0) && buffer[start] == HDLC_ESCAPE) {
                escape = HDLC_TRANSPARENCY;
            } else {
                if (len < PACKET_MAX)
                    copy [len++] = buffer[start] ^ escape;
                escape = 0;
            }
            start++;
            if (start >= end)
                return 0; /* No more data, but the frame is not complete yet. */
        }
        /* found flag.  skip past it */
        start++;
        /* check for over-short packets and silently discard, as per RFC1662 */
        if ((len < 4) || (escape != 0)) {
            len = 0; escape = 0;
            continue;
        }
        /* check, then remove the 16-bit FCS checksum field */
        if (pppfcs16 (PPPINITFCS16, copy, len) != PPPGOODFCS16)
            warn("Bad Frame Check Sequence during PPP to GRE decapsulation");
        len -= sizeof(u_int16_t);
        /* so now we have a packet of length 'len' in 'copy' */
        if ((status = cb (cl, copy, len)) < 0)
            return status; /* error-check */
        /* Great!  Let's do more! */
        len = 0; escape = 0;
    }
    return 0;
    /* No more data to process. */
}

/*** Make stripped packet into HDLC packet ************************************/
int encaps_hdlc(int fd, void *pack, unsigned int len)
{
    unsigned char *source = (unsigned char *)pack;
    unsigned char dest[2 * PACKET_MAX + 2]; /* largest expansion possible */
    unsigned int pos = 0, i;
    u_int16_t fcs;
    /* in synchronous mode there is little to do */
    if ( syncppp )
        return write(fd, source, len);
    /* asynchronous mode */
    /* Compute the FCS */
    fcs = pppfcs16(PPPINITFCS16, source, len) ^ 0xFFFF;
    /* start character */
    dest[pos++] = HDLC_FLAG;
    /* escape the payload */
    for (i = 0; i < len + 2; i++) {
        /* wacked out assignment to add FCS to end of source buffer */
        unsigned char c =
            (i < len)?source[i]:(i == len)?(fcs & 0xFF):((fcs >> 8) & 0xFF);
        if (pos >= sizeof(dest)) break; /* truncate on overflow */
        if ( (c < 0x20) || (c == HDLC_FLAG) || (c == HDLC_ESCAPE) ) {
            dest[pos++] = HDLC_ESCAPE;
            if (pos < sizeof(dest))
                dest[pos++] = c ^ 0x20;
        } else
            dest[pos++] = c;
    }
    /* tack on the end-flag */
    if (pos < sizeof(dest))
        dest[pos++] = HDLC_FLAG;
    /* now write this packet */
    return write(fd, dest, pos);
}

/*** decaps_gre ***************************************************************/
int decaps_gre (int fd, callback_t callback, int cl)
{
    unsigned char buffer[PACKET_MAX + 64 /*ip header*/];
    struct pptp_gre_header *header;
    int status, ip_len = 0;
    static int first = 1;
    unsigned int headersize;
    unsigned int payload_len;
    u_int32_t seq;

    if ((status = read (fd, buffer, sizeof(buffer))) <= 0) {
        warn("short read (%d): %s", status, strerror(errno));
        stats.rx_errors++;
        return -1;
    }
    /* strip off IP header, if present */
    if ((buffer[0] & 0xF0) == 0x40) 
        ip_len = (buffer[0] & 0xF) * 4;
    header = (struct pptp_gre_header *)(buffer + ip_len);
    /* verify packet (else discard) */
    if (    /* version should be 1 */
            ((ntoh8(header->ver) & 0x7F) != PPTP_GRE_VER) ||
            /* PPTP-GRE protocol for PPTP */
            (ntoh16(header->protocol) != PPTP_GRE_PROTO)||
            /* flag C should be clear   */
            PPTP_GRE_IS_C(ntoh8(header->flags)) ||
            /* flag R should be clear   */
            PPTP_GRE_IS_R(ntoh8(header->flags)) ||
            /* flag K should be set     */
            (!PPTP_GRE_IS_K(ntoh8(header->flags))) ||
            /* routing and recursion ctrl = 0  */
            ((ntoh8(header->flags)&0xF) != 0)) {
        /* if invalid, discard this packet */
        warn("Discarding GRE: %X %X %X %X %X %X", 
                ntoh8(header->ver)&0x7F, ntoh16(header->protocol), 
                PPTP_GRE_IS_C(ntoh8(header->flags)),
                PPTP_GRE_IS_R(ntoh8(header->flags)), 
                PPTP_GRE_IS_K(ntoh8(header->flags)),
                ntoh8(header->flags) & 0xF);
        stats.rx_invalid++;
        return 0;
    }
    /* silently discard packets not for this call */
    if (ntoh16(header->call_id) != pptp_gre_call_id) return 0;
    /* test if acknowledgement present */
    if (PPTP_GRE_IS_A(ntoh8(header->ver))) { 
        u_int32_t ack = (PPTP_GRE_IS_S(ntoh8(header->flags)))?
            header->ack:header->seq; /* ack in different place if S = 0 */
        ack = ntoh32( ack);
        if (ack > ack_recv) ack_recv = ack;
        /* also handle sequence number wrap-around  */
        if (WRAPPED(ack,ack_recv)) ack_recv = ack;
        if (ack_recv == stats.pt.seq) {
            int rtt = time_now_usecs() - stats.pt.time;
            stats.rtt = (stats.rtt + rtt) / 2;
        }
    }
    /* test if payload present */
    if (!PPTP_GRE_IS_S(ntoh8(header->flags)))
        return 0; /* ack, but no payload */
    headersize  = sizeof(*header);
    payload_len = ntoh16(header->payload_len);
    seq         = ntoh32(header->seq);
    /* no ack present? */
    if (!PPTP_GRE_IS_A(ntoh8(header->ver))) headersize -= sizeof(header->ack);
    /* check for incomplete packet (length smaller than expected) */
    if (status - headersize < payload_len) {
        warn("discarding truncated packet (expected %d, got %d bytes)",
                payload_len, status - headersize);
        stats.rx_truncated++;
        return 0; 
    }
    /* wklin modified start, 01/25/2007 */
    /* The seq# is maintained in the kernel, cannot use the seq# to determine if
     * the packet can be accepted or not. Just forward all the ppp control
     * packets to pppd, those packets should be justified there.
     */
    stats.rx_accepted++;
    first = 0;
    seq_recv = seq;
    return callback(cl, buffer + ip_len + headersize, payload_len);
#if 0
    /* check for expected sequence number */
    if ( first || (seq == seq_recv + 1)) { /* wrap-around safe */
	if ( log_level >= 2 )
            log("accepting packet %d", seq);
        stats.rx_accepted++;
        first = 0;
        seq_recv = seq;
        return callback(cl, buffer + ip_len + headersize, payload_len);
    /* out of order, check if the number is too low and discard the packet. 
     * (handle sequence number wrap-around, and try to do it right) */
    } else if ( seq < seq_recv + 1 || WRAPPED(seq_recv, seq) ) {
	if ( log_level >= 1 )
            log("discarding duplicate or old packet %d (expecting %d)",
                seq, seq_recv + 1);
        stats.rx_underwin++;
    /* sequence number too high, is it reasonably close? */
    } else if ( seq < seq_recv + MISSING_WINDOW ||
                WRAPPED(seq, seq_recv + MISSING_WINDOW) ) {
	stats.rx_buffered++;
        if ( log_level >= 1 )
            log("%s packet %d (expecting %d, lost or reordered)",
                disable_buffer ? "accepting" : "buffering",
                seq, seq_recv+1);
        if ( disable_buffer ) {
            seq_recv = seq;
            stats.rx_lost += seq - seq_recv - 1;
            return callback(cl, buffer + ip_len + headersize, payload_len);
        } else {
            pqueue_add(seq, buffer + ip_len + headersize, payload_len);
	}
    /* no, packet must be discarded */
    } else {
	if ( log_level >= 1 )
            warn("discarding bogus packet %d (expecting %d)", 
		 seq, seq_recv + 1);
        stats.rx_overwin++;
    }
#endif /* wklin modified end, 01/25/2007 */
    return 0;
}

/*** dequeue_gre **************************************************************/
int dequeue_gre (callback_t callback, int cl)
{
    pqueue_t *head;
    int status;
    /* process packets in the queue that either are expected or have 
     * timed out. */
    head = pqueue_head();
    while ( head != NULL &&
            ( (head->seq == seq_recv + 1) || /* wrap-around safe */ 
              (pqueue_expiry_time(head) <= 0) 
            )
          ) {
        /* if it is timed out... */
        if (head->seq != seq_recv + 1 ) {  /* wrap-around safe */          
            stats.rx_lost += head->seq - seq_recv - 1;
	    if (log_level >= 2)
                log("timeout waiting for %d packets", head->seq - seq_recv - 1);
        }
	if (log_level >= 2)
            log("accepting %d from queue", head->seq);
        seq_recv = head->seq;
        status = callback(cl, head->packet, head->packlen);
        pqueue_del(head);
        if (status < 0)
            return status;
        head = pqueue_head();
    }
    return 0;
}

/*** encaps_gre ***************************************************************/
int encaps_gre (int fd, void *pack, unsigned int len)
{
    union {
        struct pptp_gre_header header;
        unsigned char buffer[PACKET_MAX + sizeof(struct pptp_gre_header)];
    } u;
    //static u_int32_t seq = 1; /* first sequence number sent must be 1 */
    unsigned int header_len;
    int rc;
    /*  added start, Winster Chan, 06/26/2006 */
    static u_int32_t kerseq; /* Sequence number got from kernel by ioctl() */
    /*  added end, Winster Chan, 06/26/2006 */

    /* package this up in a GRE shell. */
    u.header.flags       = hton8 (PPTP_GRE_FLAG_K);
    u.header.ver         = hton8 (PPTP_GRE_VER);
    u.header.protocol    = hton16(PPTP_GRE_PROTO);
    u.header.payload_len = hton16(len);
    u.header.call_id     = hton16(pptp_gre_peer_call_id);
    /* special case ACK with no payload */
    if (pack == NULL) {
        if (ack_sent != seq_recv) {
            u.header.ver |= hton8(PPTP_GRE_FLAG_A);
            u.header.payload_len = hton16(0);
            /* ack is in odd place because S == 0 */
            u.header.seq = hton32(seq_recv);
            ack_sent = seq_recv;
            rc = write(fd, &u.header, sizeof(u.header) - sizeof(u.header.seq));
            if (rc < 0) {
                stats.tx_failed++;
            } else if (rc < sizeof(u.header) - sizeof(u.header.seq)) {
                stats.tx_short++;
            } else {
                stats.tx_acks++;
            }
            return rc;
        } else return 0; /* we don't need to send ACK */
    } /* explicit brace to avoid ambiguous `else' warning */
    /* send packet with payload */
    u.header.flags |= hton8(PPTP_GRE_FLAG_S);
    /*  modified start, Winster Chan, 06/26/2006 */
    //u.header.seq    = hton32(seq);
    if (pox_fd >= 0) {
    	if (ioctl(pox_fd, PPTPIOCGGRESEQ, &kerseq) == -1) {
            warn("Couldn't get GRE sequence number");
        }
	}
	else
        warn("Socket not opened");
    u.header.seq    = hton32(kerseq);
    /*  modified end, Winster Chan, 06/26/2006 */
    if (ack_sent != seq_recv) { /* send ack with this message */
        u.header.ver |= hton8(PPTP_GRE_FLAG_A);
        u.header.ack  = hton32(seq_recv);
        ack_sent = seq_recv;
        header_len = sizeof(u.header);
    } else { /* don't send ack */
        header_len = sizeof(u.header) - sizeof(u.header.ack);
    }
    if (header_len + len >= sizeof(u.buffer)) {
        stats.tx_oversize++;
        return 0; /* drop this, it's too big */
    }
    /* copy payload into buffer */
    memcpy(u.buffer + header_len, pack, len);
    /* record and increment sequence numbers */
    /*  modified start, Winster Chan, 06/26/2006 */
    //seq_sent = seq; seq++;
    /*
     * Note: the kerseq(kernel sequence number) is maintained by
     *       pptp kernel driver
     */
    seq_sent = kerseq;
    /*  modified end, Winster Chan, 06/26/2006 */
    /* write this baby out to the net */
    /* print_packet(2, u.buffer, header_len + len); */
    rc = write(fd, u.buffer, header_len + len);
    if (rc < 0) {
        stats.tx_failed++;
    } else if (rc < header_len + len) {
        stats.tx_short++;
    } else {
        stats.tx_sent++;
        stats.pt.seq  = seq_sent;
        stats.pt.time = time_now_usecs();
    }
    return rc;
}
