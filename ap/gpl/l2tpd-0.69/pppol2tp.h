#ifndef __PPPOL2TP_H
#define __PPPOL2TP_H

#include <asm/types.h>

#define PX_PROTO_OL2TP 2 /* Now L2TP also, by MJ. */

#define PPPIOCDETACH    _IOW('t', 60, int)  /* detach from ppp unit/chan */
#define PPPIOCCONNECT   _IOW('t', 58, int)  /* attach to ppp channel */
#define PPPIOCATTCHAN   _IOW('t', 56, int)  /* attach to ppp channel */
#define PPPIOCGCHAN     _IOR('t', 55, int)  /* get ppp channel number */


/* Structure used to bind() the socket to a particular socket & tunnel */
struct pppol2tp_addr
{
    pid_t   pid;            /* pid that owns the fd.
                     * 0 => current */
    int fd;         /* FD of UDP socket to use */

    struct sockaddr_in addr;    /* IP address and port to send to */

    __u16 s_tunnel, s_session;  /* For matching incoming packets */
    __u16 d_tunnel, d_session;  /* For sending outgoing packets */
};


/* , add start by MJ. for pppol2tp. 01/21/2010 */
struct sockaddr_pppol2tp {
    sa_family_t     sa_family;      /* address family, AF_PPPOX */
    unsigned int    sa_protocol;    /* protocol identifier */
    struct pppol2tp_addr pppol2tp;
}__attribute__ ((packed));
/* , add end by MJ. for pppol2tp. 01/21/2010 */

#endif

