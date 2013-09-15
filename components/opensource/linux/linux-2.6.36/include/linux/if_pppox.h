/***************************************************************************
 * Linux PPP over X - Generic PPP transport layer sockets
 * Linux PPP over Ethernet (PPPoE) Socket Implementation (RFC 2516) 
 *
 * This file supplies definitions required by the PPP over Ethernet driver
 * (pppox.c).  All version information wrt this file is located in pppox.c
 *
 * License:
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#ifndef __LINUX_IF_PPPOX_H
#define __LINUX_IF_PPPOX_H


#include <linux/types.h>
#include <asm/byteorder.h>

#ifdef  __KERNEL__
#include <linux/if_ether.h>
#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/ppp_channel.h>
#endif /* __KERNEL__ */
#include <linux/if_pppol2tp.h>

/* For user-space programs to pick up these definitions
 * which they wouldn't get otherwise without defining __KERNEL__
 */
#ifndef AF_PPPOX
#define AF_PPPOX	24
#define PF_PPPOX	AF_PPPOX
#endif /* !(AF_PPPOX) */

/************************************************************************ 
 * PPPoE addressing definition 
 */ 
typedef __be16 sid_t;
struct pppoe_addr{ 
       sid_t           sid;                    /* Session identifier */ 
       unsigned char   remote[ETH_ALEN];       /* Remote address */ 
       char            dev[IFNAMSIZ];          /* Local device to use */ 
}; 
 
/************************************************************************ 
* PPTP addressing definition
*/
struct pptp_addr {
#if 0 
	unsigned short  call_id;
	struct in_addr  sin_addr;
#else
	unsigned char   remote[ETH_ALEN];       /* Remote address */
	unsigned short  cid;                    /* PPTP call id */
	unsigned short  pcid;                   /* PPTP peer call id */
	unsigned long   seq_num;                /* Seq number of PPP packet */
	unsigned long   ack_num;                /* Ack number of PPP packet */
	unsigned long   srcaddr;                /* Source IP address */
	unsigned long   dstaddr;                /* Destination IP address */
	char            dev[IFNAMSIZ];          /* Local device to use */
#endif
};

/************************************************************************
 * Protocols supported by AF_PPPOX
 */
#define PX_PROTO_OE    0 /* Currently just PPPoE */
#define PX_PROTO_TP    1 /* Add PPTP */
#define PX_PROTO_PPTP  (PX_PROTO_TP)
#define PX_PROTO_OL2TP 2 /* Add L2TP */
#define PX_MAX_PROTO   3

struct sockaddr_pppox {
	sa_family_t     sa_family;            /* address family, AF_PPPOX */
	unsigned int    sa_protocol;          /* protocol identifier */
	union {
		struct pppoe_addr  pppoe;
#if 0 
		struct pptp_addr   pptp;
#endif
	} sa_addr;
} __attribute__((packed));


struct sockaddr_pptpox {
    sa_family_t     sa_family;            /* address family, AF_PPPOX */
    unsigned int    sa_protocol;          /* protocol identifier */
    union{
        struct pptp_addr    pptp;
    }sa_addr;
}__attribute__ ((packed));

/* The use of the above union isn't viable because the size of this
 * struct must stay fixed over time -- applications use sizeof(struct
 * sockaddr_pppox) to fill it. We use a protocol specific sockaddr
 * type instead.
 */
struct sockaddr_pppol2tp {
	sa_family_t     sa_family;      /* address family, AF_PPPOX */
	unsigned int    sa_protocol;    /* protocol identifier */
	struct pppol2tp_addr pppol2tp;
} __attribute__((packed));

/* The L2TPv3 protocol changes tunnel and session ids from 16 to 32
 * bits. So we need a different sockaddr structure.
 */
struct sockaddr_pppol2tpv3 {
	sa_family_t     sa_family;      /* address family, AF_PPPOX */
	unsigned int    sa_protocol;    /* protocol identifier */
	struct pppol2tpv3_addr pppol2tp;
} __attribute__((packed));

/*********************************************************************
 *
 * ioctl interface for defining forwarding of connections
 *
 ********************************************************************/

#define PPPOEIOCSFWD	_IOW(0xB1 ,0, size_t)
#define PPPOEIOCDFWD	_IO(0xB1 ,1)
/*#define PPPOEIOCGFWD	_IOWR(0xB1,2, size_t)*/
/*  added start, pptp, Winster Chan, 06/26/2006 */
#define PPTPIOCSFWD     _IOW(0xB1 ,0, size_t)
#define PPTPIOCDFWD     _IO(0xB1 ,1)
/*  added end, pptp, Winster Chan, 06/26/2006 */

/*  wklin added start, 12/09/2010 */
#define PPTPIOCGGRESEQ  _IOR('t', 54, unsigned long)	/* get GRE sequence number */
/*  wklin added end, 12/09/2010 */

/* Codes to identify message types */
#define PADI_CODE	0x09
#define PADO_CODE	0x07
#define PADR_CODE	0x19
#define PADS_CODE	0x65
#define PADT_CODE	0xa7
struct pppoe_tag {
	__be16 tag_type;
	__be16 tag_len;
	char tag_data[0];
} __attribute__ ((packed));

/* Tag identifiers */
#define PTT_EOL		__cpu_to_be16(0x0000)
#define PTT_SRV_NAME	__cpu_to_be16(0x0101)
#define PTT_AC_NAME	__cpu_to_be16(0x0102)
#define PTT_HOST_UNIQ	__cpu_to_be16(0x0103)
#define PTT_AC_COOKIE	__cpu_to_be16(0x0104)
#define PTT_VENDOR 	__cpu_to_be16(0x0105)
#define PTT_RELAY_SID	__cpu_to_be16(0x0110)
#define PTT_SRV_ERR     __cpu_to_be16(0x0201)
#define PTT_SYS_ERR  	__cpu_to_be16(0x0202)
#define PTT_GEN_ERR  	__cpu_to_be16(0x0203)

struct pppoe_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	__u8 ver : 4;
	__u8 type : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	__u8 type : 4;
	__u8 ver : 4;
#else
#error	"Please fix <asm/byteorder.h>"
#endif
	__u8 code;
	__be16 sid;
	__be16 length;
	struct pppoe_tag tag[0];
} __attribute__((packed));

/* Length of entire PPPoE + PPP header */
#define PPPOE_SES_HLEN	8
/* Socket options */
#define PPTP_SO_TIMEOUT 1
#define PPTP_SO_WINDOW  2     

/*  added start, pptp, Winster Chan, 06/26/2006 */
/* IP PROTOCOL HEADER */

/* GRE Protocol field */
#define IP_PROTOCOL_ICMP    0x01
#define IP_PROTOCOL_GRE     0x2f

struct pptp_ip_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
    __u8    ihl:4,      /* Header length */
        version:4;      /* Version */
#elif defined (__BIG_ENDIAN_BITFIELD)
    __u8    version:4,  /* Version */
        ihl:4;          /* Header length */
#else
#error	"Please fix <asm/byteorder.h>"
#endif
    __u8	tos;        /* Differentiated services field */
    __u16	tot_len;    /* Total length */
    __u16	id;         /* Identification */
    __u16	frag_off;   /* Fragment flags & offset */
    __u8	ttl;        /* Time to live */
    __u8	protocol;   /* Protocol: GRE(0x2f), ICMP(0x01) */
    __u16	check;      /* Header checksum */
    __u32	saddr;      /* Source IP address */
    __u32	daddr;      /* Destination IP address */
    /*The options start here. */
} __attribute__ ((packed));

/* GRE PROTOCOL HEADER */

/* GRE Version field */
//#define GRE_VERSION_1701	0x0
#define GRE_VERSION_PPTP	0x1

/* GRE Protocol field */
#define GRE_PROTOCOL_PPTP	__constant_htons(0x880B)

/* GRE Flags */
#define GRE_FLAG_C		0x80
#define GRE_FLAG_R		0x40
#define GRE_FLAG_K		0x20
#define GRE_FLAG_S		0x10
#define GRE_FLAG_A		0x80

#define GRE_IS_C(f)	((f)&GRE_FLAG_C)
#define GRE_IS_R(f)	((f)&GRE_FLAG_R)
#define GRE_IS_K(f)	((f)&GRE_FLAG_K)
#define GRE_IS_S(f)	((f)&GRE_FLAG_S)
#define GRE_IS_A(f)	((f)&GRE_FLAG_A)

/* GRE is a mess: Four different standards */
/* modified GRE header for PPTP */
struct pptp_gre_hdr {
    __u8  flags;        /* bitfield */
    __u8  version;      /* should be GRE_VERSION_PPTP */
    __u16 protocol;     /* should be GRE_PROTOCOL_PPTP */
    __u16 payload_len;  /* size of ppp payload, not inc. gre header */
    __u16 call_id;      /* peer's call_id for this session */
    __u32 seq;          /* sequence number.  Present if S==1 */
    __u32 ack;          /* seq number of highest packet recieved by */
                        /*  sender in this session */
} __attribute__ ((packed));

/* PPTP packet (IP & GRE) header */
struct pptp_hdr {
    struct pptp_ip_hdr      iphdr;      /* IP header */
    struct pptp_gre_hdr     grehdr;     /* GRE header */
} __attribute__ ((packed));
/*  added end, pptp, Winster Chan, 06/26/2006 */

#ifdef __KERNEL__
#include <linux/skbuff.h>

static inline struct pppoe_hdr *pppoe_hdr(const struct sk_buff *skb)
{
	return (struct pppoe_hdr *)skb_network_header(skb);
}

struct pppoe_opt {
	struct net_device      *dev;	  /* device associated with socket*/
	int			ifindex;  /* ifindex of device associated with socket */
	struct pppoe_addr	pa;	  /* what this socket is bound to*/
	struct sockaddr_pppox	relay;	  /* what socket data will be
					     relayed to (PPPoE relaying) */
};

struct pptp_opt {
#if 0 
	struct pptp_addr src_addr;
	struct pptp_addr dst_addr;
	u32 ack_sent, ack_recv;
	u32 seq_sent, seq_recv;
	int ppp_flags;
#else
	struct net_device      *dev;	  /* device associated with socket*/
	int			ifindex;  /* ifindex of device associated with socket */
	struct pptp_addr	pa;	  /* what this socket is bound to*/
	struct sockaddr_pptpox	relay;	  /* what socket data will be
					     relayed to (PPTP relaying) */
#endif
};

#include <net/sock.h>

struct pppox_sock {
	/* struct sock must be the first member of pppox_sock */
	struct sock		sk;
	struct ppp_channel	chan;
	struct pppox_sock	*next;	  /* for hash table */
	union {
		struct pppoe_opt pppoe;
		struct pptp_opt  pptp;
	} proto;
	__be16			num;
};
#define pppoe_dev	proto.pppoe.dev
#define pppoe_ifindex	proto.pppoe.ifindex
#define pppoe_pa	proto.pppoe.pa
#define pppoe_relay	proto.pppoe.relay
#define pptp_dev	proto.pptp.dev
#define pptp_ifindex	proto.pptp.ifindex
#define pptp_pa	proto.pptp.pa
#define pptp_relay	proto.pptp.relay

static inline struct pppox_sock *pppox_sk(struct sock *sk)
{
	return (struct pppox_sock *)sk;
}

static inline struct sock *sk_pppox(struct pppox_sock *po)
{
	return (struct sock *)po;
}

struct module;

struct pppox_proto {
	int		(*create)(struct net *net, struct socket *sock);
	int		(*ioctl)(struct socket *sock, unsigned int cmd,
				 unsigned long arg);
	struct module	*owner;
};

extern int register_pppox_proto(int proto_num, struct pppox_proto *pp);
extern void unregister_pppox_proto(int proto_num);
extern void pppox_unbind_sock(struct sock *sk);/* delete ppp-channel binding */
extern int pppox_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);

/* PPPoX socket states */
enum {
    PPPOX_NONE		= 0,  /* initial state */
    PPPOX_CONNECTED	= 1,  /* connection established ==TCP_ESTABLISHED */
    PPPOX_BOUND		= 2,  /* bound to ppp device */
    PPPOX_RELAY		= 4,  /* forwarding is enabled */
    PPPOX_ZOMBIE	= 8,  /* dead, but still bound to ppp device */
    PPPOX_DEAD		= 16  /* dead, useless, please clean me up!*/
};

#endif /* __KERNEL__ */

#endif /* !(__LINUX_IF_PPPOX_H) */
