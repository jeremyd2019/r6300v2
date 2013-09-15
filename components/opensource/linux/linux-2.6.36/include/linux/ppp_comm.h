
#include <linux/ppp_channel.h>
#include <net/sock.h>

/* Fxcn port-S Wins, 0714-09 */
//#define uchar   unsigned char
//#define ushort  unsigned short
//#define ulong   unsigned long
//#define uint    unsigned int
/* Fxcn port-E Wins, 0714-09 */

#define PPP_NETWORK_LAYER   0x3FFF
#define PPP_NW_LAYER        0x3F

#define PPP_PROTOCOL_PPPOE  1
#define PPP_PROTOCOL_PPTP   2
#ifdef INCLUDE_L2TP
#define PPP_PROTOCOL_L2TP   3 /*  added, zacker, 02/01/2010, @l2tp */
#endif

/*  added start, pptp, Winster Chan, 06/26/2006 */
struct ppp_info {
    unsigned int protocol_type;
    struct ppp_channel *pchan;
};

struct sock_info {
    unsigned int protocol_type;
    struct sock *sk;
};

#define ETH_MAC_LEN     6
#define SEQ_PRESENT     0x01
#define ACK_PRESENT     0x02

#define PPP_PROTO_IP    0x0021  /* Internet Protocol */

struct num_info {
    unsigned char flag_a_s;     /* Flag for sequence & acknowledgement number */
    unsigned long gre_seq;      /* Sequence number of GRE header */
    unsigned long gre_ack;      /* Acknowledgement number of GRE header */
};

struct hdr_info {
    unsigned char eth_smac[ETH_MAC_LEN];    /* Source MAC address */
    unsigned char eth_dmac[ETH_MAC_LEN];    /* Destination MAC address */
    unsigned long ip_saddr;     /* Source IP address */
    unsigned long ip_daddr;     /* Destination IP address */
    struct num_info seq_ack;
};

struct mac_hdr {
    unsigned char smac[ETH_MAC_LEN];    /* Source MAC address */
    unsigned char dmac[ETH_MAC_LEN];    /* Destination MAC address */
    unsigned short type;
};

struct addr_info {
    unsigned long src_addr;     /* Source IP address */
    unsigned long dst_addr;     /* Destination IP address */
};
/*  added end, pptp, Winster Chan, 06/26/2006 */
