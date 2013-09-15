/** -*- linux-c -*- ***********************************************************
 * Linux Point-to-Point Tunneling Protocol (PPPoX/PPTP) Sockets
 *
 * PPPoX --- Generic PPP encapsulation socket family
 * PPTP  --- Point-to-Point Tunneling Protocol (RFC 2637)
 *
 *
 * Version:	0.1.0
 *
 ******************************************************************************
 * 0915-09 :	Create PPTP module for Linux 2.6.22
 ******************************************************************************
 * Author:  Winster Chan <winster.wh.chan@.com>
 * Contributors:
 *
 * License:
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 */

#include <linux/string.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/net.h>
#include <linux/inetdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/init.h>
#include <linux/if_ether.h>
#include <linux/if_pppox.h>
#include <linux/ppp_channel.h>
#include <linux/ppp_defs.h>
#include <linux/if_ppp.h>
#include <linux/notifier.h>
#include <linux/file.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include <net/sock.h>       /* for struct sock */
#include <net/inet_sock.h>  /* for struct inet_sock */
#include <net/ip.h>         /* for func ip_select_ident() */
#include <net/route.h>      /* for struct rtable and routing */
#include <asm/uaccess.h>
#include <linux/ppp_comm.h>

/* define "portable" htons, etc. */
#define hton8(x)  (x)
#define ntoh8(x)  (x)
#define hton16(x) htons(x)
#define ntoh16(x) ntohs(x)
#define hton32(x) htonl(x)
#define ntoh32(x) ntohl(x)

#define NEED_SEQ        1
#define NO_SEQ          0
#define NEED_ACK        1
#define NO_ACK          0

#define INIT_SEQ        1   /* Base number of PPTP sequence */

static unsigned long call_id;       /* Call ID for local client */
static unsigned long peer_call_id;  /* Call ID for peer server */
static unsigned long seq_number = INIT_SEQ;
static unsigned long ack_number = 0;
static unsigned long src_ip_addr; /* 10.0.0.22 = 0x0A000016 */
static unsigned long dst_ip_addr; /* 10.0.0.238 = 0x0A0000EE */

#define PPTP_HASH_BITS 4
#define PPTP_HASH_SIZE (1<<PPTP_HASH_BITS)

static struct ppp_channel_ops pptp_chan_ops;

static int pptp_ioctl(struct socket *sock, unsigned int cmd, unsigned long arg);
static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb);
static int __pptp_xmit(struct sock *sk, struct sk_buff *skb);

static const struct proto_ops pptp_ops;
static DEFINE_RWLOCK(pptp_hash_lock);

extern char *nvram_get(const char *name);

static inline int cmp_2_addr(struct pptp_addr *a, struct pptp_addr *b)
{
	return (a->srcaddr == b->srcaddr &&
		(memcmp(a->remote, b->remote, ETH_ALEN) == 0));
}

static inline int cmp_addr(struct pptp_addr *a, unsigned long ipaddr, char *addr)
{
	return (a->srcaddr == ipaddr &&
		(memcmp(a->remote,addr,ETH_ALEN) == 0));
}

static int hash_item(unsigned long ipaddr, unsigned char *addr)
{
	char hash = 0;
	int i, j;

	for (i = 0; i < ETH_ALEN ; ++i) {
		for (j = 0; j < 8/PPTP_HASH_BITS ; ++j) {
			hash ^= addr[i] >> ( j * PPTP_HASH_BITS );
		}
	}

	for (i = 0; i < (sizeof(unsigned long)*8) / PPTP_HASH_BITS ; ++i)
		hash ^= ipaddr >> (i*PPTP_HASH_BITS);

	return hash & ( PPTP_HASH_SIZE - 1 );
}

/* zeroed because its in .bss */
static struct pppox_sock *item_hash_table[PPTP_HASH_SIZE];

/**********************************************************************
 *
 *  Set/get/delete/rehash items  (internal versions)
 *
 **********************************************************************/
static struct pppox_sock *__get_item(unsigned long ipaddr, unsigned char *addr, int ifindex)
{
	int hash = hash_item(ipaddr, addr);
	struct pppox_sock *ret;

	ret = item_hash_table[hash];

	while (ret && !(cmp_addr(&ret->pptp_pa, ipaddr, addr) && ret->pptp_ifindex == ifindex))
		ret = ret->next;

	return ret;
}

static int __set_item(struct pppox_sock *po)
{
	int hash = hash_item(po->pptp_pa.srcaddr, po->pptp_pa.remote);
	struct pppox_sock *ret;

	ret = item_hash_table[hash];
	while (ret) {
		if (cmp_2_addr(&ret->pptp_pa, &po->pptp_pa) && ret->pptp_ifindex == po->pptp_ifindex)
			return -EALREADY;

		ret = ret->next;
	}

	po->next = item_hash_table[hash];
	item_hash_table[hash] = po;

	return 0;
}

static struct pppox_sock *__delete_item(unsigned long ipaddr, char *addr, int ifindex)
{
	int hash = hash_item(ipaddr, addr);
	struct pppox_sock *ret, **src;

	ret = item_hash_table[hash];
	src = &item_hash_table[hash];

	while (ret) {
		if (cmp_addr(&ret->pptp_pa, ipaddr, addr) && ret->pptp_ifindex == ifindex) {
			*src = ret->next;
			break;
		}

		src = &ret->next;
		ret = ret->next;
	}

	return ret;
}

/**********************************************************************
 *
 *  Set/get/delete/rehash items
 *
 **********************************************************************/
static inline struct pppox_sock *get_item(unsigned long ipaddr,
					 unsigned char *addr, int ifindex)
{
	struct pppox_sock *po;

	read_lock_bh(&pptp_hash_lock);
	po = __get_item(ipaddr, addr, ifindex);
	if (po)
		sock_hold(sk_pppox(po));
	read_unlock_bh(&pptp_hash_lock);

	return po;
}

static inline struct pppox_sock *get_item_by_addr(struct sockaddr_pptpox *sp)
{
	struct net_device *dev;
	int ifindex;

	dev = dev_get_by_name(&init_net, sp->sa_addr.pptp.dev);
	if(!dev)
		return NULL;
	ifindex = dev->ifindex;
	dev_put(dev);
	return get_item(sp->sa_addr.pptp.srcaddr, sp->sa_addr.pptp.remote, ifindex);
}

static inline struct pppox_sock *delete_item(unsigned long ipaddr, char *addr, int ifindex)
{
	struct pppox_sock *ret;

	write_lock_bh(&pptp_hash_lock);
	ret = __delete_item(ipaddr, addr, ifindex);
	write_unlock_bh(&pptp_hash_lock);

	return ret;
}


/***************************************************************************
 *
 *  Handler for device events.
 *  Certain device events require that sockets be unconnected.
 *
 **************************************************************************/

static void pptp_flush_dev(struct net_device *dev)
{
	int hash;
	BUG_ON(dev == NULL);

	write_lock_bh(&pptp_hash_lock);
	for (hash = 0; hash < PPTP_HASH_SIZE; hash++) {
		struct pppox_sock *po = item_hash_table[hash];

		while (po != NULL) {
			struct sock *sk = sk_pppox(po);
			if (po->pptp_dev != dev) {
				po = po->next;
				continue;
			}
			po->pptp_dev = NULL;
			dev_put(dev);


			/* We always grab the socket lock, followed by the
			 * pptp_hash_lock, in that order.  Since we should
			 * hold the sock lock while doing any unbinding,
			 * we need to release the lock we're holding.
			 * Hold a reference to the sock so it doesn't disappear
			 * as we're jumping between locks.
			 */

			sock_hold(sk);

			write_unlock_bh(&pptp_hash_lock);
			lock_sock(sk);

			if (sk->sk_state & (PPPOX_CONNECTED | PPPOX_BOUND)) {
				pppox_unbind_sock(sk);
				sk->sk_state = PPPOX_ZOMBIE;
				sk->sk_state_change(sk);
			}

			release_sock(sk);
			sock_put(sk);

			/* Restart scan at the beginning of this hash chain.
			 * While the lock was dropped the chain contents may
			 * have changed.
			 */
			write_lock_bh(&pptp_hash_lock);
			po = item_hash_table[hash];
		}
	}
	write_unlock_bh(&pptp_hash_lock);
}

static int pptp_device_event(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	struct net_device *dev = (struct net_device *) ptr;

	/* Only look at sockets that are using this specific device. */
	switch (event) {
	case NETDEV_CHANGEMTU:
		/* A change in mtu is a bad thing, requiring
		 * LCP re-negotiation.
		 */

	case NETDEV_GOING_DOWN:
	case NETDEV_DOWN:
		/* Find every socket on this device and kill it. */
		pptp_flush_dev(dev);
		break;

	default:
		break;
	};

	return NOTIFY_DONE;
}


static struct notifier_block pptp_notifier = {
	.notifier_call = pptp_device_event,
};


/*  added start, pptp, Winster Chan, 06/26/2006 */
/************************************************************************
 *
 * Get the length of IP header to be stripped off.
 * receive function for internal use.
 *
 ***********************************************************************/
static int pptp_get_ip_header(char *pktdata)
{
    struct pptp_ip_hdr *piphdr = (struct pptp_ip_hdr *)pktdata;

    if (piphdr->protocol == IP_PROTOCOL_GRE)
        return (piphdr->ihl * 4);
    else
        return 0;
}

/************************************************************************
 *
 * Get the length of GRE header to be stripped off.
 * receive function for internal use.
 *
 ***********************************************************************/
static int pptp_get_gre_header(char *pktdata)
{
    struct pptp_gre_hdr *pgrehdr = (struct pptp_gre_hdr *)pktdata;
    int grehdrlen = 8;

    if (pgrehdr->protocol != GRE_PROTOCOL_PPTP)
        return 0;

    /* Check if sequence number presents */
    if (GRE_IS_S(pgrehdr->flags))
        grehdrlen += 4;

    /* Check if acknowledgement number presents */
    if (GRE_IS_A(pgrehdr->version))
        grehdrlen += 4;

    return grehdrlen;
}

/************************************************************************
 *
 * Padding the IP header before transmitting
 * xmit function for internal use.
 *
 ***********************************************************************/
static void pptp_fill_ip_header(struct sock *sk,
                struct sk_buff *skb,
                struct pptp_ip_hdr *iphdr,
                int total_len)
{
    struct rtable *rt = (struct rtable *)skb->_skb_refdst;
	struct inet_sock *inet = inet_sk(sk);

    iphdr->version = 4; /* Version */
    iphdr->ihl = 5; /* Header length (5*4)=20 */
    iphdr->tos = inet->tos; /* Differentiated services field */
    //iphdr->tot_len = hton16(skb->len + grehdrlen + 20 + 2); /* Total length */
    iphdr->tot_len = hton16(total_len); /* Total length */
    /* Fragment flags & offset */
    //iphdr->frag_off = hton16(IP_DF); /* Don't fragment */
    iphdr->frag_off = 0;
    /* Select identification */
    ip_select_ident((struct iphdr *)iphdr, &rt->dst, sk);
    iphdr->ttl = 255; /* Time to live (64), maximum is 255 */
    iphdr->protocol = IP_PROTOCOL_GRE; /* Protocol: GRE(0x2f) */
    iphdr->saddr = src_ip_addr; /* Source IP address */
    iphdr->daddr = dst_ip_addr; /* Destination IP address */
    /* Header checksum */
    iphdr->check = 0;
    iphdr->check = ip_fast_csum((unsigned char *) iphdr, iphdr->ihl);
}

/************************************************************************
 *
 * Padding the GRE header before transmitting
 * xmit function for internal use.
 *
 ***********************************************************************/
static int pptp_fill_gre_header(unsigned long callid,
                struct pptp_gre_hdr *grehdr,
                int payloadlen, int need_seq, int need_ack)
{
    int hdrlen = 8; /* GRE header length */

    grehdr->flags = 0x00; /* bitfield */
    grehdr->flags |= GRE_FLAG_K; /* Key present */
    if (need_seq)
        grehdr->flags |= GRE_FLAG_S; /* Sequence number present */

    grehdr->version = GRE_VERSION_PPTP;
    if (need_ack)
        grehdr->version |= GRE_FLAG_A; /* Acknowledgement number present */

    grehdr->protocol = GRE_PROTOCOL_PPTP;
    /*
     * size of ppp payload, not inc. gre header;
     * besides the 0xFF03 was appened in front of the ppp packet.
     * So the total payload length should += 2.
     */
    //grehdr->payload_len = hton16(skb->len + 2);
    grehdr->payload_len = hton16(payloadlen);
    grehdr->call_id = hton16(callid); /* peer's call_id for this session */
    if (GRE_IS_S(grehdr->flags)) {
        grehdr->seq = hton32(seq_number); /* sequence number.  Present if S==1 */
        seq_number++;
        hdrlen += 4;
    }
    if (GRE_IS_A(grehdr->version)) {
        ack_number += 1;
        grehdr->ack = hton32(ack_number); /* seq number of highest packet recieved by */
        hdrlen += 4;
    }
    return hdrlen;
}
/*  added end, pptp, Winster Chan, 06/26/2006 */

/************************************************************************
 *
 * Do the real work of receiving a PPTP GRE frame.
 *
 ***********************************************************************/
static int pptp_rcv_core(struct sock *sk, struct sk_buff *skb)
{
	struct pppox_sock *po = pppox_sk(sk);
	struct pppox_sock *relay_po;
	int hdrlen, iphdrlen, grehdrlen;
    int ppp_hdr_start;  /*  added pling 04/18/2011 */

    iphdrlen = pptp_get_ip_header((char *)skb_network_header(skb));
    grehdrlen = pptp_get_gre_header((char *)skb_network_header(skb) + iphdrlen);
    hdrlen = iphdrlen + grehdrlen + 2; /* 2 is length for 0xFF03 */

    /*  added start pling  04/18/2011 */
    /* Russia PPTP server (mpd3) issue:
     * Sometimes the server send packet without the 'FF03' 
     * in PPP header.
     */
    ppp_hdr_start = iphdrlen + grehdrlen;
    if (skb->data[ppp_hdr_start] != 0xFF  || 
        skb->data[ppp_hdr_start+1] != 0x03) {
#if (defined RU_VERSION)
        if (1)
#elif (defined WW_VERSION)
        if (strcmp(nvram_get("gui_region"), "Russian") == 0)
#else   /* Other FW, don't apply this patch */
       if (0)
#endif
        {
            hdrlen -= 2;
        }
    }
    /*  added end pling  04/18/2011 */

	if (sk->sk_state & PPPOX_BOUND) {
#if 0
        struct pptp_ip_hdr *piphdr = 
            (struct pptp_ip_hdr *) skb_network_header(skb);
        struct pptp_gre_hdr *pgrehdr =
            (struct pptp_gre_hdr *) ((char *)skb_network_header(skb) 
            + iphdrlen);
        int len = ntoh16(pgrehdr->payload_len - 2);
#endif
        skb_pull(skb, hdrlen);
#if 0
		skb_pull_rcsum(skb, sizeof(struct pptp_hdr));
		if (pskb_trim_rcsum(skb, len))
			goto abort_kfree;
#endif

		ppp_input(&po->chan, skb);
	} else if (sk->sk_state & PPPOX_RELAY) {
		relay_po = get_item_by_addr(&po->pptp_relay);

		if (relay_po == NULL)
			goto abort_kfree;

		if ((sk_pppox(relay_po)->sk_state & PPPOX_CONNECTED) == 0)
			goto abort_put;

		skb_pull(skb, hdrlen);
		if (!__pptp_xmit(sk_pppox(relay_po), skb))
			goto abort_put;
	} else {
		if (sock_queue_rcv_skb(sk, skb))
			goto abort_kfree;
	}

	return NET_RX_SUCCESS;

abort_put:
	sock_put(sk_pppox(relay_po));

abort_kfree:
	kfree_skb(skb);
	return NET_RX_DROP;
}

/************************************************************************
 *
 * Receive wrapper called in BH context.
 *
 ***********************************************************************/
static int pptp_rcv(struct sk_buff *skb,
		     struct net_device *dev,
		     struct packet_type *pt,
		     struct net_device *orig_dev)

{
	struct pppox_sock *po;
	int iphdrlen, grehdrlen, hdrlen;
	struct pptp_ip_hdr *piphdr;
	struct pptp_gre_hdr *pgrehdr;

    iphdrlen = pptp_get_ip_header((char *)skb_network_header(skb));
    grehdrlen = pptp_get_gre_header((char *)skb_network_header(skb) + iphdrlen);
    hdrlen = iphdrlen + grehdrlen + 2; /* 2 is length for 0xFF03 */

#if 0
	if (!pskb_may_pull(skb, hdrlen))
		goto drop;

	if (!(skb = skb_share_check(skb, GFP_ATOMIC)))
		goto out;
#endif

    piphdr = (struct pptp_ip_hdr *) skb_network_header(skb);
    pgrehdr = (struct pptp_gre_hdr *)((char *)piphdr + iphdrlen);

	po = get_item(piphdr->daddr, eth_hdr(skb)->h_source, dev->ifindex);
	if (po != NULL)
		return sk_receive_skb(sk_pppox(po), skb, 0);
//drop:
	kfree_skb(skb);
//out:
	return NET_RX_DROP;
}

/*******************************************************************
 * ETH_P_IP = 0x0800, @ include/linux/if_ether.h
 * ETH_P_PPTP_GRE = 0x082F, @ include/linux/if_ether.h
 ******************************************************************/
static struct packet_type pptp_gre_ptype = {
	.type	= __constant_htons(ETH_P_PPTP_GRE),
	.func	= pptp_rcv,
};

static struct proto pptp_sk_proto = {
	.name	  = "PPTP",
	.owner	  = THIS_MODULE,
	.obj_size = sizeof(struct pppox_sock),
};

/***********************************************************************
 *
 * Initialize a new struct sock.
 *
 **********************************************************************/

static int pptp_create(struct net *net, struct socket *sock)
{
	struct sock *sk;

	sk = sk_alloc(net, PF_PPPOX, GFP_KERNEL, &pptp_sk_proto);
	if (!sk)
		return -ENOMEM;

	sock_init_data(sock, sk);

    //pptp_sock.sk = sk;
    //pptp_sock.protocol_type = PPP_PROTOCOL_PPTP;
    //ppp_import_sock_info(&pptp_sock);
	sock->state	= SS_UNCONNECTED;
	sock->ops	= &pptp_ops;

	sk->sk_backlog_rcv	= pptp_rcv_core;
	sk->sk_state		= PPPOX_NONE;
	sk->sk_type		= SOCK_STREAM;
	sk->sk_family		= PF_PPPOX;
	sk->sk_protocol		= PX_PROTO_TP;

	return 0;
}
static int pptp_release(struct socket *sock)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po;

	if (!sk)
		return 0;

	lock_sock(sk);
	if (sock_flag(sk, SOCK_DEAD)){
		release_sock(sk);
		return -EBADF;
	}

	pppox_unbind_sock(sk);

	/* Signal the death of the socket. */
	sk->sk_state = PPPOX_DEAD;


	/* Write lock on hash lock protects the entire "po" struct from
	 * concurrent updates via pptp_flush_dev. The "po" struct should
	 * be considered part of the hash table contents, thus protected
	 * by the hash table lock */
	write_lock_bh(&pptp_hash_lock);

	po = pppox_sk(sk);
	if (po->pptp_pa.srcaddr) {
		__delete_item(po->pptp_pa.srcaddr,
			      po->pptp_pa.remote, po->pptp_ifindex);
	}

	if (po->pptp_dev) {
		dev_put(po->pptp_dev);
		po->pptp_dev = NULL;
	}

	write_unlock_bh(&pptp_hash_lock);

	sock_orphan(sk);
	sock->sk = NULL;

	skb_queue_purge(&sk->sk_receive_queue);
	release_sock(sk);
	sock_put(sk);

	return 0;
}

extern void dev_import_call_id(unsigned long call_id, unsigned long peer_call_id);

static int pptp_connect(struct socket *sock, struct sockaddr *uservaddr,
		  int sockaddr_len, int flags)
{
	struct sock *sk = sock->sk;
	struct net_device *dev;
	struct sockaddr_pptpox *sp = (struct sockaddr_pptpox *) uservaddr;
	struct pppox_sock *po = pppox_sk(sk);
	int error;

	lock_sock(sk);

	error = -EINVAL;
	if (sp->sa_protocol != PX_PROTO_TP)
		goto end;

	/* Check for already bound sockets */
	error = -EBUSY;
	if ((sk->sk_state & PPPOX_CONNECTED) && sp->sa_addr.pptp.srcaddr)
		goto end;

	/* Check for already disconnected sockets, on attempts to disconnect */
	error = -EALREADY;
	if ((sk->sk_state & PPPOX_DEAD) && !sp->sa_addr.pptp.srcaddr )
		goto end;

	error = 0;
	if (po->pptp_pa.srcaddr) {
		pppox_unbind_sock(sk);

		/* Delete the old binding */
		delete_item(po->pptp_pa.srcaddr,po->pptp_pa.remote,po->pptp_ifindex);

		if(po->pptp_dev)
			dev_put(po->pptp_dev);

		memset(sk_pppox(po) + 1, 0,
		       sizeof(struct pppox_sock) - sizeof(struct sock));

		sk->sk_state = PPPOX_NONE;
	}

	/* Don't re-bind if srcaddr==0 */
	if (sp->sa_addr.pptp.srcaddr != 0) {
        call_id = sp->sa_addr.pptp.cid;
        peer_call_id = sp->sa_addr.pptp.pcid;
		dev = dev_get_by_name(&init_net, sp->sa_addr.pptp.dev);

		error = -ENODEV;
		if (!dev)
			goto end;

		po->pptp_dev = dev;
		po->pptp_ifindex = dev->ifindex;

		write_lock_bh(&pptp_hash_lock);
		if (!(dev->flags & IFF_UP)){
			write_unlock_bh(&pptp_hash_lock);
			goto err_put;
		}

		memcpy(&po->pptp_pa,
		       &sp->sa_addr.pptp,
		       sizeof(struct pptp_addr));

        src_ip_addr = sp->sa_addr.pptp.srcaddr;
        dst_ip_addr = sp->sa_addr.pptp.dstaddr;

		error = __set_item(po);
		write_unlock_bh(&pptp_hash_lock);
		if (error < 0)
			goto err_put;

        /* struct (pptp_hdr) = struct (pptp_ip_hdr) + 
                               struct (pptp_gre_hdr) */
		po->chan.hdrlen = (sizeof(struct pptp_hdr) + 2 +
				   dev->hard_header_len); /* 0xFF03 len = 2 */

		po->chan.mtu = dev->mtu - sizeof(struct pptp_hdr);
		po->chan.private = sk;
		po->chan.ops = &pptp_chan_ops;

		error = ppp_register_channel(&po->chan);
		if (error)
			goto err_put;

		sk->sk_state = PPPOX_CONNECTED;
        dev_import_addr_info(&sp->sa_addr.pptp.srcaddr,
                             &sp->sa_addr.pptp.dstaddr);

		/*  added start pling 03/20/2012 */
		/* export the call id so that dev.c know 
		 * such info */
		dev_import_call_id(call_id, peer_call_id);
		/*  added end pling 03/20/2012 */
	}

	po->num = sp->sa_addr.pptp.srcaddr;

end:
	release_sock(sk);
	return error;
err_put:
	if (po->pptp_dev) {
		dev_put(po->pptp_dev);
		po->pptp_dev = NULL;
	}
	goto end;
}


static int pptp_getname(struct socket *sock, struct sockaddr *uaddr,
		  int *usockaddr_len, int peer)
{
	int len = sizeof(struct sockaddr_pptpox);
	struct sockaddr_pptpox sp;

	sp.sa_family	= AF_PPPOX;
	sp.sa_protocol	= PX_PROTO_TP;
	memcpy(&sp.sa_addr.pptp, &pppox_sk(sock->sk)->pptp_pa,
	       sizeof(struct pptp_addr));

	memcpy(uaddr, &sp, len);

	*usockaddr_len = len;

	return 0;
}


static int pptp_ioctl(struct socket *sock, unsigned int cmd,
		unsigned long arg)
{
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	int val = 0;
	int err = 0;

	switch (cmd) {
	case PPPIOCGMRU:
		err = -ENXIO;

		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (put_user(po->pptp_dev->mtu -
			     (sizeof(struct pptp_hdr) + 2) -
			     PPP_HDRLEN,
			     (int __user *) arg))
			break;
		err = 0;
		break;

	case PPPIOCSMRU:
		err = -ENXIO;
		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (get_user(val,(int __user *) arg))
			break;

		if (val < (po->pptp_dev->mtu
			   - (sizeof(struct pptp_hdr) + 2)
			   - PPP_HDRLEN))
			err = 0;
		else
			err = -EINVAL;
		break;

	case PPPIOCSFLAGS:
		err = -EFAULT;
		if (get_user(val, (int __user *) arg))
			break;
		err = 0;
		break;

	case PPTPIOCSFWD:
	{
		struct pppox_sock *relay_po;

		err = -EBUSY;
		if (sk->sk_state & (PPPOX_BOUND | PPPOX_ZOMBIE | PPPOX_DEAD))
			break;

		err = -ENOTCONN;
		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		/* PPTP address from the user specifies an outbound
		   PPTP address which frames are forwarded to */
		err = -EFAULT;
		if (copy_from_user(&po->pptp_relay,
				   (void __user *)arg,
				   sizeof(struct sockaddr_pptpox)))
			break;

		err = -EINVAL;
		if (po->pptp_relay.sa_family != AF_PPPOX ||
		    po->pptp_relay.sa_protocol!= PX_PROTO_TP)
			break;

		/* Check that the socket referenced by the address
		   actually exists. */
		relay_po = get_item_by_addr(&po->pptp_relay);

		if (!relay_po)
			break;

		sock_put(sk_pppox(relay_po));
		sk->sk_state |= PPPOX_RELAY;
		err = 0;
		break;
	}

	case PPTPIOCDFWD:
		err = -EALREADY;
		if (!(sk->sk_state & PPPOX_RELAY))
			break;

		sk->sk_state &= ~PPPOX_RELAY;
		err = 0;
		break;

    /*  added start, pptp, Winster Chan, 06/26/2006 */
	case PPTPIOCGGRESEQ:
		err = -ENXIO;

		if (!(sk->sk_state & PPPOX_CONNECTED))
			break;

		err = -EFAULT;
		if (put_user((unsigned long)seq_number,
			     (int __user *)arg))
			break;
		err = 0;
		seq_number++;
		break;
    /*  added end, pptp, Winster Chan, 06/26/2006 */

	default:;
	};

	return err;
}


static int pptp_sendmsg(struct kiocb *iocb, struct socket *sock,
		  struct msghdr *m, size_t total_len)
{
	struct sk_buff *skb;
	struct sock *sk = sock->sk;
	struct pppox_sock *po = pppox_sk(sk);
	int error;
	struct net_device *dev;
	char *start;
    int grehdrlen = 0;
    int hdrlen = sizeof(struct pptp_ip_hdr) + 
                 sizeof(struct pptp_gre_hdr) + 2;
    unsigned char tphdr[hdrlen]; /* Tunnel protocol header */
    unsigned char *ptphdr; /* Pointer for tunnel protocol header */

	if (sock_flag(sk, SOCK_DEAD) || !(sk->sk_state & PPPOX_CONNECTED)) {
		error = -ENOTCONN;
		goto end;
	}

    /*
     * In this case, we set the IP header size to fixed size 
     * 20 bytes, (without any optional field).
     */
    /* Fill in the GRE header data */
    grehdrlen = pptp_fill_gre_header(peer_call_id,
                    (struct pptp_gre_hdr *)&tphdr[20],
                    (total_len + 2), NEED_SEQ, NO_ACK);
    /*
     * IP header length = 20, 0xFF03 length = 2,
     * total header length = IP header + GRE header + 2
     */
    hdrlen = 20 + grehdrlen;
    tphdr[hdrlen++] = 0xFF; /* Address field of PPP header */
    tphdr[hdrlen++] = 0x03; /* Control field of PPP header */
    /* Fill in the IP header data */
    pptp_fill_ip_header(sk, skb, (struct pptp_ip_hdr *)&tphdr[0],
                    (total_len + hdrlen));

	lock_sock(sk);

	dev = po->pptp_dev;

	error = -EMSGSIZE;
 	if (total_len > (dev->mtu + dev->hard_header_len))
		goto end;


	skb = sock_wmalloc(sk, total_len + dev->hard_header_len + 32 + hdrlen,
			   0, GFP_KERNEL);
	if (!skb) {
		error = -ENOMEM;
		goto end;
	}

	/* Reserve space for headers. */
	skb_reserve(skb, dev->hard_header_len);
	skb_reset_network_header(skb);

	skb->dev = dev;

	skb->priority = sk->sk_priority;
	skb->protocol = __constant_htons(ETH_P_IP);

    /*
     * Push IP header, GRE header, and PPP address(0xFF) + 
     * Control(0x03) in one shot.
     */
	ptphdr = (unsigned char *) skb_put(skb, total_len + hdrlen);
	start = (char *) ptphdr + hdrlen;

	error = memcpy_fromiovec(start, m->msg_iov, total_len);

	if (error < 0) {
		kfree_skb(skb);
		goto end;
	}

	error = total_len;
    dev_hard_header(skb, dev, ETH_P_IP,
			po->pptp_pa.remote, NULL, total_len);

    memcpy(ptphdr, tphdr, hdrlen);

	dev_queue_xmit(skb);

end:
	release_sock(sk);
	return error;
}


/************************************************************************
 *
 * xmit function for internal use.
 *
 ***********************************************************************/
/*  wklin modified start, 08/05/2010 @pptp throughput */
static int __pptp_xmit(struct sock *sk, struct sk_buff *skb)
{
	struct pppox_sock *po = pppox_sk(sk);
	struct net_device *dev = po->pptp_dev;
	int headroom = skb_headroom(skb);
	int data_len = skb->len;
	struct sk_buff *skb2;
    int grehdrlen = 0;
    int hdrlen = sizeof(struct pptp_ip_hdr) + 
                 sizeof(struct pptp_gre_hdr) + 2;
    unsigned char tphdr[hdrlen]; /* Tunnel protocol header */
    unsigned char *ptphdr; /* Pointer for tunnel protocol header */

	if (sock_flag(sk, SOCK_DEAD) || !(sk->sk_state & PPPOX_CONNECTED))
		goto abort;

    /*
     * In this case, we set the IP header size to fixed size 
     * 20 bytes, (without any optional field).
     */
    /* Fill in the GRE header data */
    grehdrlen = pptp_fill_gre_header(peer_call_id,
                    (struct pptp_gre_hdr *)&tphdr[20],
                    (skb->len + 2), NEED_SEQ, NO_ACK);
    /*
     * IP header length = 20, 0xFF03 length = 2,
     * total header length = IP header + GRE header + 2
     */
    hdrlen = 20 + grehdrlen;
    tphdr[hdrlen++] = 0xFF; /* Address field of PPP header */
    tphdr[hdrlen++] = 0x03; /* Control field of PPP header */
    /* Fill in the IP header data */
    pptp_fill_ip_header(sk, skb, (struct pptp_ip_hdr *)&tphdr[0],
                    (skb->len + hdrlen));

	if (!dev)
		goto abort;

	/* Copy the skb if there is no space for the header. */
	/* Length + 2 is for PPP address 0xFF and Control 0x03 */
	if (headroom < (hdrlen + dev->hard_header_len)) {
		skb2 = dev_alloc_skb(32 + skb->len + hdrlen +
				     dev->hard_header_len);

		if (skb2 == NULL)
			goto abort;

		skb_reserve(skb2, dev->hard_header_len + hdrlen);
		skb_copy_from_linear_data(skb, skb_put(skb2, skb->len),
					  skb->len);
        kfree_skb(skb);
        skb = skb2;
	} 

    /*
     * Push IP header, GRE header, and PPP address(0xFF) + 
     * Control(0x03) in one shot.
     */
    ptphdr = (unsigned char *) skb_push(skb, hdrlen);
    memcpy(ptphdr, tphdr, hdrlen);
    skb->protocol = __constant_htons(ETH_P_IP);

	skb_reset_network_header(skb);

	skb->dev = dev;

			 
	dev_hard_header(skb, dev, ETH_P_IP,
			 po->pptp_pa.remote, NULL, data_len);

	/* We're transmitting skb2, and assuming that dev_queue_xmit
	 * will free it.  The generic ppp layer however, is expecting
	 * that we give back 'skb' (not 'skb2') in case of failure,
	 * but free it in case of success.
	 */

	if (dev_queue_xmit(skb) < 0)
		goto abort;
	return 1;

abort:
	return 0;
}
/*  wklin moified end, 08/05/2010 @pptp throughput */


/************************************************************************
 *
 * xmit function called by generic PPP driver
 * sends PPP frame over PPTP socket
 *
 ***********************************************************************/
static int pptp_xmit(struct ppp_channel *chan, struct sk_buff *skb)
{
	struct sock *sk = (struct sock *) chan->private;
	return __pptp_xmit(sk, skb);
}

static struct ppp_channel_ops pptp_chan_ops = {
	.start_xmit = pptp_xmit,
};

static int pptp_recvmsg(struct kiocb *iocb, struct socket *sock,
		  struct msghdr *m, size_t total_len, int flags)
{
	struct sock *sk = sock->sk;
	struct sk_buff *skb;
	int error = 0;
	int len;
    struct pptp_ip_hdr *piphdr;
    struct pptp_gre_hdr *pgrehdr;
    int iphdrlen, grehdrlen;

	if (sk->sk_state & PPPOX_BOUND) {
		error = -EIO;
		goto end;
	}

	skb = skb_recv_datagram(sk, flags & ~MSG_DONTWAIT,
				flags & MSG_DONTWAIT, &error);

	if (error < 0)
		goto end;

	m->msg_namelen = 0;

	if (skb) {
		error = 0;
        iphdrlen = 
            pptp_get_ip_header((char *)skb_network_header(skb));
        grehdrlen = 
            pptp_get_gre_header((char *)skb_network_header(skb) + 
                                iphdrlen);
        piphdr = (struct pptp_ip_hdr *) skb_network_header(skb);
        pgrehdr = 
            (struct pptp_gre_hdr *) ((char *)skb_network_header(skb) 
                                     + iphdrlen);
        len = ntoh16(pgrehdr->payload_len - 2);

		error = memcpy_toiovec(m->msg_iov,
		            ((unsigned char *) pgrehdr + grehdrlen + 2), len);
		if (error == 0)
			error = len;
	}

	kfree_skb(skb);
end:
	return error;
}

static int pptp_proc_info(char *buffer, char **start, off_t offset, int length, int *eof, void *data)
{
	struct pppox_sock *po;
	int len = 0;
	off_t pos = 0;
	off_t begin = 0;
	int size;
	int i;

	len += sprintf(buffer,
		       "Id       Address              Device\n");
	pos = len;

	write_lock_bh(&pptp_hash_lock);

	for (i = 0; i < PPTP_HASH_SIZE; i++) {
		po = item_hash_table[i];
		while (po) {
			char *dev = po->pptp_pa.dev;

			size = sprintf(buffer + len,
				       "%08X %02X:%02X:%02X:%02X:%02X:%02X %8s\n",
				       (unsigned int)po->pptp_pa.srcaddr,
				       po->pptp_pa.remote[0],
				       po->pptp_pa.remote[1],
				       po->pptp_pa.remote[2],
				       po->pptp_pa.remote[3],
				       po->pptp_pa.remote[4],
				       po->pptp_pa.remote[5],
				       dev);
			len += size;
			pos += size;
			if (pos < offset) {
				len = 0;
				begin = pos;
			}

			if (pos > offset + length)
				break;

			po = po->next;
		}

		if (po)
			break;
  	}
	write_unlock_bh(&pptp_hash_lock);

  	*start = buffer + (offset - begin);
  	len -= (offset - begin);
  	if (len > length)
  		len = length;
	if (len < 0)
		len = 0;
  	return len;
}

/* ->ioctl are set at pppox_create */

static const struct proto_ops pptp_ops = {
    .family		= AF_PPPOX,
    .owner		= THIS_MODULE,
    .release		= pptp_release,
    .bind		= sock_no_bind,
    .connect		= pptp_connect,
    .socketpair		= sock_no_socketpair,
    .accept		= sock_no_accept,
    .getname		= pptp_getname,
    .poll		= datagram_poll,
    .listen		= sock_no_listen,
    .shutdown		= sock_no_shutdown,
    .setsockopt		= sock_no_setsockopt,
    .getsockopt		= sock_no_getsockopt,
    .sendmsg		= pptp_sendmsg,
    .recvmsg		= pptp_recvmsg,
    .mmap		= sock_no_mmap,
    .ioctl		= pppox_ioctl,
};

static struct pppox_proto pptp_proto = {
    .create	= pptp_create,
    .ioctl	= pptp_ioctl,
    .owner	= THIS_MODULE,
};


static int __init pptp_init(void)
{
#if 0
	int err = proto_register(&pptp_sk_proto, 0);

	if (err)
		goto out;

 	err = register_pppox_proto(PX_PROTO_TP, &pptp_proto);
	if (err)
		goto out_unregister_pptp_proto;

	err = pptp_proc_init();
	if (err)
		goto out_unregister_pppox_proto;

	dev_add_pack(&pptp_gre_ptype);
	register_netdevice_notifier(&pptp_notifier);

    /* Init sequence & acknowledgement values */
    seq_number = INIT_SEQ;
    ack_number = 0;

    /* Clear flags */
    ppp_ch_imported = 0;
out:
	return err;
out_unregister_pppox_proto:
	unregister_pppox_proto(PX_PROTO_TP);
out_unregister_pptp_proto:
	proto_unregister(&pptp_sk_proto);
	goto out;
#else
 	int err = register_pppox_proto(PX_PROTO_TP, &pptp_proto);

	if (err == 0) {
		dev_add_pack(&pptp_gre_ptype);
		register_netdevice_notifier(&pptp_notifier);
		create_proc_read_entry("pptp", 0, init_net.proc_net, pptp_proc_info, NULL);

        /* Init sequence & acknowledgement values */
        seq_number = INIT_SEQ;
        ack_number = 0;
	}
	return err;
#endif
}

static void __exit pptp_exit(void)
{
	unregister_pppox_proto(PX_PROTO_TP);
	dev_remove_pack(&pptp_gre_ptype);
	unregister_netdevice_notifier(&pptp_notifier);
#if 0
	remove_proc_entry("net/pptp", NULL);
	proto_unregister(&pptp_sk_proto);
#else
	remove_proc_entry("net/pptp", NULL);
#endif
}

module_init(pptp_init);
module_exit(pptp_exit);

MODULE_AUTHOR("Winster Chan <winster.wh.chan@foxconn>");
MODULE_DESCRIPTION("PPP tunnel protocol driver");
MODULE_LICENSE("GPL");
//MODULE_ALIAS_NETPROTO(PF_PPPOX);
