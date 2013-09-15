/***************************************************************************
***
***    Copyright 2005  Hon Hai Precision Ind. Co. Ltd.
***    All Rights Reserved.
***    No portions of this material shall be reproduced in any form without the
***    written permission of Hon Hai Precision Ind. Co. Ltd.
***
***    All information contained in this document is Hon Hai Precision Ind.
***    Co. Ltd. company private, proprietary, and trade secret property and
***    are protected by international intellectual property laws and treaties.
***
****************************************************************************/

# include <sys/types.h>
# include <sys/socket.h>

/* Length of interface name.  */
#define IF_NAMESIZE	16

/* Device mapping structure. I'd just gone off and designed a
   beautiful scheme using only loadable modules with arguments for
   driver options and along come the PCMCIA people 8)

   Ah well. The get() side of this is good for WDSETUP, and it'll be
   handy for debugging things. The set side is fine for now and being
   very small might be worth keeping for clean configuration.  */

struct ifmap
  {
    unsigned long int mem_start;
    unsigned long int mem_end;
    unsigned short int base_addr;
    unsigned char irq;
    unsigned char dma;
    unsigned char port;
    /* 3 bytes spare */
  };

/* Interface request structure used for socket ioctl's.  All interface
   ioctl's must have parameter definitions which begin with ifr_name.
   The remainder may be interface specific.  */

struct ifreq
  {
# define IFHWADDRLEN	6
# define IFNAMSIZ	IF_NAMESIZE
    union
      {
        char ifrn_name[IFNAMSIZ];	/* Interface name, e.g. "en0".  */
      } ifr_ifrn;

    union
      {
        struct sockaddr ifru_addr;
        struct sockaddr ifru_dstaddr;
        struct sockaddr ifru_broadaddr;
        struct sockaddr ifru_netmask;
        struct sockaddr ifru_hwaddr;
        short int ifru_flags;
        int ifru_ivalue;
        int ifru_mtu;
        struct ifmap ifru_map;
        char ifru_slave[IFNAMSIZ];	/* Just fits the size */
        char ifru_newname[IFNAMSIZ];
        __caddr_t ifru_data;
      } ifr_ifru;
  };
# define ifr_name	ifr_ifrn.ifrn_name	/* interface name 	*/
# define ifr_hwaddr	ifr_ifru.ifru_hwaddr	/* MAC address 		*/
# define ifr_addr	ifr_ifru.ifru_addr	/* address		*/
# define ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-p lnk	*/
# define ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address	*/
# define ifr_netmask	ifr_ifru.ifru_netmask	/* interface net mask	*/
# define ifr_flags	ifr_ifru.ifru_flags	/* flags		*/
# define ifr_metric	ifr_ifru.ifru_ivalue	/* metric		*/
# define ifr_mtu	ifr_ifru.ifru_mtu	/* mtu			*/
# define ifr_map	ifr_ifru.ifru_map	/* device map		*/
# define ifr_slave	ifr_ifru.ifru_slave	/* slave device		*/
# define ifr_data	ifr_ifru.ifru_data	/* for use by interface	*/
# define ifr_ifindex	ifr_ifru.ifru_ivalue    /* interface index      */
# define ifr_bandwidth	ifr_ifru.ifru_ivalue	/* link bandwidth	*/
# define ifr_qlen	ifr_ifru.ifru_ivalue	/* queue length		*/
# define ifr_newname	ifr_ifru.ifru_newname	/* New name		*/
# define _IOT_ifreq	_IOT(_IOTS(char),IFNAMSIZ,_IOTS(char),16,0,0)
# define _IOT_ifreq_short _IOT(_IOTS(char),IFNAMSIZ,_IOTS(short),1,0,0)
# define _IOT_ifreq_int	_IOT(_IOTS(char),IFNAMSIZ,_IOTS(int),1,0,0)


/*  added start, Winster Chan, 06/26/2006 */
#include <linux/types.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
/*#include <linux/if_pppox.h>*/

#include <linux/if_ether.h> /* ETH_ALEN */
#include <net/if.h> /* IFNAMSIZ */
#include <sys/file.h>


#define PX_PROTO_OE     0 /* Currently just PPPoE */
#define PX_PROTO_TP     1 /* Add PPTP */
#define PX_MAX_PROTO    2

#define ETH_ALEN        6 /* Ethernet MAC address length */
#define IPV4_LEN        4 /* IP V4 length */

/* This definition must refer to linux/if_ppp.h */
#define PPTPIOCGGRESEQ  _IOR('t', 54, unsigned long)	/* get GRE sequence number */

/************************************************************************
 * PPTP addressing definition
 */
struct pptp_addr{
    unsigned char   remote[ETH_ALEN];       /* Remote address */
    unsigned short  sid;                    /* PPPoE session id */
    unsigned short  cid;                    /* PPTP call id */
    unsigned short  pcid;                   /* PPTP peer call id */
    unsigned long   seq_num;                /* Seq number of PPP packet */
    unsigned long   ack_num;                /* Ack number of PPP packet */
    unsigned long   srcaddr;                /* Source IP address */
    unsigned long   dstaddr;                /* Destination IP address */
    char            dev[IFNAMSIZ];          /* Local device to use */
};

struct sockaddr_pptpox {
    sa_family_t     sa_family;            /* address family, AF_PPPOX */
    unsigned int    sa_protocol;          /* protocol identifier */
    union{
        struct pptp_addr    pptp;
    }sa_addr;
}__attribute__ ((packed));

/************************************************************************
 * PPPoE addressing definition
 */
typedef __u16 sid_t;
struct pppoe_addr{
       sid_t           sid;                    /* Session identifier */
       unsigned char   remote[ETH_ALEN];       /* Remote address */
       char            dev[IFNAMSIZ];          /* Local device to use */
};

struct sockaddr_pppox {
       sa_family_t     sa_family;            /* address family, AF_PPPOX */
       unsigned int    sa_protocol;          /* protocol identifier */
       union{
               struct pppoe_addr       pppoe;
       }sa_addr;
}__attribute__ ((packed));


void pptp_pppox_open(int *poxfd, int *pppfd);
int pptp_pppox_connect(int *poxfd, int *pppfd); /*  wklin modified,
                                                   07/31/2007 */
struct sockaddr_pppox pptp_pppox_get_info(void);
void pptp_pppox_release(int *poxfd, int *pppfd);

/*  added end, pptp, Winster Chan, 06/26/2006 */
