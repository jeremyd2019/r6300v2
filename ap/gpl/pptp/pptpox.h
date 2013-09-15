/*  added start, pptp, Winster Chan, 06/26/2006 */
#include <linux/if_ether.h> /* ETH_ALEN */
#include <net/if.h> /* IFNAMSIZ */

#define PX_PROTO_OE     0 /* Currently just PPPoE */
#define PX_PROTO_TP     1 /* Add PPTP */
#define PX_MAX_PROTO    2

#define ETH_ALEN        6 /* Ethernet MAC address length */
#define IPV4_LEN        4 /* IP V4 length */
//#define IP_ADDR_MAX     16

/* This definition must refer to linux/if_ppp.h */
#define PPTPIOCGGRESEQ  _IOR('t', 54, unsigned long)	/* get GRE sequence number */

/************************************************************************
 * PPTP addressing definition
 */
struct pptp_addr{
    unsigned char   remote[ETH_ALEN];       /* Remote address */
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


void pptp_pppox_open(int *poxfd, int *pppfd);
void pptp_pppox_connect(int *poxfd, int *pppfd,
        u_int16_t call_id, u_int16_t peer_call_id);
struct sockaddr_pptpox pptp_pppox_get_info(void);
void pptp_pppox_release(int *poxfd, int *pppfd);

/*  added end, pptp, Winster Chan, 06/26/2006 */
