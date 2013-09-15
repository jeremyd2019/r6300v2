/*
 * Copyright (C) International Business Machines  Corp., 2003
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* Author: Shirley Ma, xma@us.ibm.com */

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"

static void
get_if_prefix(struct nlmsghdr *nlm, int nlm_len, int request,
	      struct dhcp6_if *ifp)
{
	struct rtmsg *rtm = (struct rtmsg *)NLMSG_DATA(nlm);
	struct rtattr *rta;
	size_t rtasize, rtapayload;
	void *rtadata;
	struct ra_info *rainfo;
	char addr[64];

	if (rtm->rtm_family != AF_INET6 || nlm->nlmsg_type != request)
		return;

	if (!(rtm->rtm_flags & RTM_F_PREFIX)) 
		return;

	rtasize = NLMSG_PAYLOAD(nlm, nlm_len) - NLMSG_ALIGN(sizeof(*rtm));
	rta = (struct rtattr *) (((char *) NLMSG_DATA(nlm)) +
	      NLMSG_ALIGN(sizeof(*rtm)));
	if (!RTA_OK(rta, rtasize))
		return;
	rtadata = RTA_DATA(rta);
	rtapayload = RTA_PAYLOAD(rta);
	switch(rta->rta_type) {
	case RTA_OIF:
		break;
	default:
		break; 
	}
	switch (rta->rta_type) {	
	case RTA_DST:
		rainfo = (struct ra_info *)malloc(sizeof(*rainfo));
		if (rainfo == NULL)
			return;
		memset(rainfo, 0, sizeof(rainfo));
		memcpy(&(rainfo->prefix), (struct in6_addr *)rtadata, 
		       sizeof(struct in6_addr));
		rainfo->plen = rtm->rtm_dst_len;
		if (ifp->ralist == NULL) {
			ifp->ralist = rainfo;
			rainfo->next = NULL;
		} else {
			struct ra_info *ra, *ra_prev;
			ra_prev = ifp->ralist;
			for (ra = ifp->ralist; ra; ra = ra->next) {
				if (rainfo->plen >= ra->plen) {
					if (ra_prev == ra) {
						ifp->ralist = rainfo;
						rainfo->next = ra;
					} else {
						ra_prev->next = rainfo;
						rainfo->next = ra;
					}
					break;
				} else {
					if (ra->next == NULL) {
						ra->next = rainfo;
						rainfo->next = NULL;
						break;
					} else {
						ra_prev = ra;
						continue;
					}
				}
			}
		}
		inet_ntop(AF_INET6, &(rainfo->prefix), addr, INET6_ADDRSTRLEN);
		dprintf(LOG_DEBUG, "get prefix address %s", addr);
		dprintf(LOG_DEBUG, "get prefix plen %d",rtm->rtm_dst_len);
		break;
	case RTA_CACHEINFO:
		dprintf(LOG_DEBUG, "prefix route life time is %d\n",
		      ((struct rta_cacheinfo *)rtadata)->rta_expires);
		break;
	default:
		break;
	}
	return;
}

static void
get_if_flags(struct nlmsghdr *nlm, int nlm_len, int request,
	     struct dhcp6_if *ifp) 
{
	struct ifinfomsg *ifim = (struct ifinfomsg *)NLMSG_DATA(nlm);
	struct rtattr *rta, *rta1;
	size_t rtasize, rtasize1, rtapayload;
	void *rtadata;

	dprintf(LOG_DEBUG, "get_if_flags called");

	if (ifim->ifi_family != AF_INET6 || nlm->nlmsg_type != request)
		return;
	if (ifim->ifi_index != ifp->ifid)
		return;
	rtasize = NLMSG_PAYLOAD(nlm, nlm_len) - NLMSG_ALIGN(sizeof(*ifim));
	for (rta = (struct rtattr *) (((char *) NLMSG_DATA(nlm)) +
	     NLMSG_ALIGN(sizeof(*ifim))); RTA_OK(rta, rtasize);
	     rta = RTA_NEXT(rta, rtasize)) {
	rtadata = RTA_DATA(rta);
	rtapayload = RTA_PAYLOAD(rta);
	
	switch(rta->rta_type) {
	case IFLA_IFNAME:
		break;
#ifdef IFLA_PROTINFO
	case IFLA_PROTINFO:
		rtasize1 = rta->rta_len;
		for (rta1 = (struct rtattr *)rtadata; RTA_OK(rta1, rtasize1);
		     rta1 = RTA_NEXT(rta1, rtasize1)) {
			void *rtadata1 = RTA_DATA(rta1);
			size_t rtapayload1= RTA_PAYLOAD(rta1);
			switch(rta1->rta_type) {
			case IFLA_INET6_CACHEINFO:
				break;
			case IFLA_INET6_FLAGS:
				/* flags for IF_RA_MANAGED/IF_RA_OTHERCONF */
				ifp->ra_flag = *((u_int32_t *)rtadata1);
				if (*((u_int32_t *)rtadata1) & IF_RA_MANAGED)
					dprintf(LOG_DEBUG, 
						"interface managed flags set");
				if (*((u_int32_t *)rtadata1) & IF_RA_OTHERCONF)
					dprintf(LOG_DEBUG,
						"interface otherconf flags set");
				break;
			default:
				break;
			}
		}
		break;
#endif
	default:
		break;
	}
	}
	return;
}

static int
open_netlink_socket()
{
	struct sockaddr_nl nl_addr;
	int sd;

	dprintf(LOG_DEBUG, "open_netlink_socket called");
	sd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (sd < 0)
		return -1;
	memset(&nl_addr, 0, sizeof(nl_addr));
	nl_addr.nl_family = AF_NETLINK;
	if (bind(sd, (struct sockaddr *)&nl_addr, sizeof(nl_addr)) < 0) {
		dprintf(LOG_ERR, "netlink bind error");
		close(sd);
		return -1;
	}
	return sd;
}

static int
netlink_send_rtmsg(int sd, int request, int flags, int seq)
{
	struct sockaddr_nl nl_addr;
	struct nlmsghdr *nlm_hdr;
	struct rtmsg *rt_msg;
	char buf[NLMSG_ALIGN (sizeof (struct nlmsghdr)) +
		  NLMSG_ALIGN (sizeof (struct rtmsg))];
	int status;
	
	memset(&buf, 0, sizeof(buf));
	dprintf(LOG_DEBUG, "netlink_send_rtmsg called");

	nlm_hdr = (struct nlmsghdr *)buf;
	nlm_hdr->nlmsg_len = NLMSG_LENGTH (sizeof (*rt_msg));
	nlm_hdr->nlmsg_type = request;
	nlm_hdr->nlmsg_flags = flags | NLM_F_REQUEST;
	nlm_hdr->nlmsg_pid = getpid();
	nlm_hdr->nlmsg_seq = seq;

	memset(&nl_addr, 0, sizeof(nl_addr));
	nl_addr.nl_family = AF_NETLINK;

	rt_msg = (struct rtmsg *)NLMSG_DATA(nlm_hdr);
	rt_msg->rtm_family = AF_INET6;
	rt_msg->rtm_flags = 0;
	rt_msg->rtm_flags |= RTM_F_PREFIX;

	status = sendto(sd, (void *)nlm_hdr, nlm_hdr->nlmsg_len, 0,
		(struct sockaddr *)&nl_addr, sizeof(nl_addr)); 
	return status;
}

static int
netlink_send_rtgenmsg(int sd, int request, int flags, int seq)
{
	struct sockaddr_nl nl_addr;
	struct nlmsghdr *nlm_hdr;
	struct rtgenmsg *rt_genmsg;
	char buf[NLMSG_ALIGN (sizeof (struct nlmsghdr)) +
		  NLMSG_ALIGN (sizeof (struct rtgenmsg))];
	int status;

	memset(&buf, 0, sizeof(buf));
	dprintf(LOG_DEBUG, "netlink_send_rtgenmsg called");

	nlm_hdr = (struct nlmsghdr *)buf;
	nlm_hdr->nlmsg_len = NLMSG_LENGTH (sizeof (*rt_genmsg));
	nlm_hdr->nlmsg_type = request;
	nlm_hdr->nlmsg_flags = flags | NLM_F_REQUEST;
	nlm_hdr->nlmsg_pid = getpid();
	nlm_hdr->nlmsg_seq = seq;

	memset(&nl_addr, 0, sizeof(nl_addr));
	nl_addr.nl_family = AF_NETLINK;

	rt_genmsg = (struct rtgenmsg*)NLMSG_DATA(nlm_hdr);
	rt_genmsg->rtgen_family = AF_INET6;
	status = sendto(sd, (void *)nlm_hdr, nlm_hdr->nlmsg_len, 0,
		(struct sockaddr *)&nl_addr, sizeof(nl_addr)); 
	return status;
}

static int 
netlink_recv_rtgenmsg(int sd, int request, int seq, struct dhcp6_if *ifp)
{
	struct nlmsghdr *nlm;
	struct msghdr msgh;
	struct sockaddr_nl nl_addr;
	char *buf = NULL;
	size_t newsize = 65536, size = 0;
	int msg_len;

	dprintf(LOG_DEBUG, "netlink_recv_rtgenmsg called");

	if (seq == 0)
		seq = (int)time(NULL);
	for (;;) {
		void *newbuf = realloc(buf, newsize);
		if (newbuf == NULL) {
			msg_len = -1;
			break;
		}
		buf = newbuf;
		do { 
			struct iovec iov = {buf, newsize};
			memset(&msgh, 0, sizeof(msgh));
			msgh.msg_name = (void *)&nl_addr;
			msgh.msg_namelen = sizeof(nl_addr);
			msgh.msg_iov = &iov;
			msgh.msg_iovlen = 1;
			msg_len = recvmsg(sd, &msgh, 0);
		} while (msg_len < 0 && errno == EINTR);

		if (msg_len < 0 || msgh.msg_flags & MSG_TRUNC) {
			size = newsize;
			newsize *= 2;
			continue;
		} else if (msg_len == 0) 
			break;

		/* buf might have some data not for this request */
		for (nlm = (struct nlmsghdr *)buf; NLMSG_OK(nlm, msg_len);
		     nlm = (struct nlmsghdr *)NLMSG_NEXT(nlm, msg_len)) {
			if (nlm->nlmsg_type == NLMSG_DONE ||
			    nlm->nlmsg_type == NLMSG_ERROR) {
				dprintf(LOG_ERR, "netlink_recv_rtgenmsg error");
				goto out;
			}
			if (nlm->nlmsg_pid != getpid() ||
			    nlm->nlmsg_seq != seq)
				continue;
			if (request == RTM_NEWROUTE)
				get_if_prefix(nlm, msg_len, request, ifp);
			if (request == RTM_NEWLINK)
				get_if_flags(nlm, msg_len, request, ifp);
		}
		free(buf);
		buf = NULL;
	}
out:	if (buf)
		free(buf);
	return msg_len;
}

int
get_if_rainfo(struct dhcp6_if *ifp)
{
	int sd, status;
	int seq = time(NULL);
	
	sd = open_netlink_socket();
	if (sd < 0)
		return sd;
	status = netlink_send_rtmsg(sd, RTM_GETROUTE, NLM_F_ROOT, seq);
	if (status >= 0)
		status = netlink_recv_rtgenmsg(sd, RTM_NEWROUTE, seq, ifp);
	else
		goto out;
	status = netlink_send_rtgenmsg(sd, RTM_GETLINK, NLM_F_ROOT, seq);
	if (status >= 0)
		status = netlink_recv_rtgenmsg(sd, RTM_NEWLINK, seq, ifp);
out:	close(sd);
	return status;
}
