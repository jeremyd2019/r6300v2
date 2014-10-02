/* dnsmasq is Copyright (c) 2000 - 2003 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*/

/* Author's email: simon@thekelleys.org.uk */

#include "dnsmasq.h"

/*  wklin added start, 08/31/2007 @mpoe */
#ifdef MULTIPLE_PPPOE
int mpoe = 0; /* for multiple wan, forward dns query to all ifs */
#endif /* MULTIPLE_PPPOE */
/*  wklin added end, 08/31/2007 */

static struct frec *frec_list;

static struct frec *get_new_frec(time_t now);
static struct frec *lookup_frec(unsigned short id);
static struct frec *lookup_frec_by_sender(unsigned short id,
					  union mysockaddr *addr);
static unsigned short get_id(void);
static void get_device_id(char src_mac[], char id[]);

/*  removed start by Jenny Zhao, 12/10/2008,@Russia_PPTP new spec*/
#if 0
/*  added start, zacker, 07/29/2008,@Russia_PPTP */
extern keyword_t pptp_domain;
extern Session2_DNS pptp_dhcp_dns_tbl;

int is_pptp_dhcp_dns(unsigned long uiDNSIP)
{
    int i;
    
    for(i=0; i<pptp_dhcp_dns_tbl.iDNSCount; i++)
    {
        if(uiDNSIP == pptp_dhcp_dns_tbl.DNSEntry[i])
        {
            return 1;
        }
    }
    
    return 0;
}

int is_pptp_domain_matched(char *namebuff)
{
    keyword_t *keyword=NULL;
    int matched = 0;
    
    if (!namebuff)
        return matched;

    for (keyword = &pptp_domain; keyword; keyword=keyword->next) 
    {
        if (keyword->wildcard == 1) 
        {
            char *p;
            if (p=strcasestr(namebuff,keyword->name)) 
            {
                if (*(p+strlen(keyword->name)) == '\0') 
                {
                    matched = 1;
                    fprintf(stderr, "wildcard matched %s\n", keyword->name);
                    break;
                }
            }
        }
        else if (strcasecmp(namebuff,keyword->name) == 0)
        {
            matched = 1;
            fprintf(stderr, "matched %s\n", keyword->name);
            break;
        }
    }
    return matched;
}
/*  added end, zacker, 07/29/2008,@Russia_PPTP */
#endif
/*  removed end by Jenny Zhao, 12/10/2008,@Russia_PPTP new spec*/

/*  wklin added start, 09/03/2007 @mpoe */
#ifdef MULTIPLE_PPPOE

/*  added start Bob Guo, 10/24/2007 */

extern Session2_DNS    Session2_Dns_Tbl;

int IsSession2DNS(unsigned long uiDNSIP)
{
    int i;
    
    for(i=0; i<Session2_Dns_Tbl.iDNSCount; i++)
    {
        if(uiDNSIP == Session2_Dns_Tbl.DNSEntry[i])
        {
            return 1;
        }
    }
    
    return 0;
}

int IsDomainKeywordMatched(char *namebuff)
{
    extern keyword_t *keyword_list;
    keyword_t *keyword=NULL;
    int matched = 0;
    
    if (!namebuff)
	return matched;
	for (keyword = keyword_list; keyword; keyword=keyword->next) 
	{
        if (keyword->wildcard == 1) 
        {
            char *p;
            if (p=strcasestr(namebuff,keyword->name)) 
            {
                if (*(p+strlen(keyword->name)) == '\0') 
                {
                    matched = 1;
                    fprintf(stderr, "wildcard matched %s\n", keyword->name);
                    break;
                }
            }
        } 
        else if (strcasestr(namebuff,keyword->name)) 
        {
            matched = 1;
	        fprintf(stderr, "matched %s\n", keyword->name);
	        break;
	    }
    }
    return matched;
    
}
/*  added end Bob Guo, 10/24/2007 */
#if 0
static void private_domain_check(HEADER *header, int n, char *namebuff)
{
    extern void extract_set_addr(HEADER *, int);

    if(IsDomainKeywordMatched(namebuff) == 1)
        extract_set_addr(header, n);
    
}
#else
#define MAX_QUERY_NAME  1500
static void private_domain_check(HEADER *header, int n, char *namebuff)
{
    char qryname[MAX_QUERY_NAME], *p;
    int i, j=0;    
    extern void extract_set_addr(HEADER *, int);
    
    if ( NULL == header )
         return;
    p= (char *)header + sizeof(HEADER);
    i = *p;
    
      /* Find the domain name in questions area, and use this name to compare with the
          keyword.                weal @ March 4, 2008 */
    while (i && j < MAX_QUERY_NAME ){
        for(;i>0;i--){
            qryname[j++] = *(++p);
            }
        i = *(++p);
        if (i)
            qryname[j++] = '.';
        }

    qryname[j++] = '\0';
    
    if(IsDomainKeywordMatched(qryname) == 1)
        extract_set_addr(header, n);

}
#endif

#endif /* MULTIPLE_PPPOE */
/*  wklin added end, 09/03/2007 */
/* May be called more than once. */
void forward_init(int first)
{
  struct frec *f;
  
  if (first)
    frec_list = NULL;
  for (f = frec_list; f; f = f->next)
    f->new_id = 0;
}

/* Send a UDP packet with it's source address set as "source" 
   unless nowild is true, when we just send it with the kernel default */
static void send_from(int fd, int nowild, char *packet, int len, 
		      union mysockaddr *to, struct all_addr *source,
		      unsigned int iface)
{
  struct msghdr msg;
  struct iovec iov[1]; 
  union {
    struct cmsghdr align; /* this ensures alignment */
#if defined(IP_PKTINFO)
    char control[CMSG_SPACE(sizeof(struct in_pktinfo))];
#elif defined(IP_SENDSRCADDR)
    char control[CMSG_SPACE(sizeof(struct in_addr))];
#endif
#ifdef HAVE_IPV6
    char control6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
#endif
  } control_u;
  
  iov[0].iov_base = packet;
  iov[0].iov_len = len;

  msg.msg_control = NULL;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;
  msg.msg_name = to;
  msg.msg_namelen = sa_len(to);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  
  if (!nowild && to->sa.sa_family == AF_INET)
    {
      msg.msg_control = &control_u;
      msg.msg_controllen = sizeof(control_u);
      {
	struct cmsghdr *cmptr = CMSG_FIRSTHDR(&msg);
#if defined(IP_PKTINFO)

	struct in_pktinfo *pkt = (struct in_pktinfo *)CMSG_DATA(cmptr);
	pkt->ipi_ifindex = 0;
	pkt->ipi_spec_dst = source->addr.addr4;
	msg.msg_controllen = cmptr->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
	cmptr->cmsg_level = SOL_IP;
	cmptr->cmsg_type = IP_PKTINFO;
#elif defined(IP_SENDSRCADDR)
	struct in_addr *a = (struct in_addr *)CMSG_DATA(cmptr);
	*a = source->addr.addr4;
	msg.msg_controllen = cmptr->cmsg_len = CMSG_LEN(sizeof(struct in_addr));
	cmptr->cmsg_level = IPPROTO_IP;
	cmptr->cmsg_type = IP_SENDSRCADDR;
#endif
      }
    }

#ifdef HAVE_IPV6
  if (to->sa.sa_family == AF_INET6)
    {
      msg.msg_control = &control_u;
      msg.msg_controllen = sizeof(control_u);
      {
	struct cmsghdr *cmptr = CMSG_FIRSTHDR(&msg);
	struct in6_pktinfo *pkt = (struct in6_pktinfo *)CMSG_DATA(cmptr);
	pkt->ipi6_ifindex = iface; /* Need iface for IPv6 to handle link-local addrs */
	pkt->ipi6_addr = source->addr.addr6;
	msg.msg_controllen = cmptr->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmptr->cmsg_type = IPV6_PKTINFO;
	cmptr->cmsg_level = IPV6_LEVEL;
      }
    }
#endif
  
  /* certain Linux kernels seem to object to setting the source address in the IPv6 stack
     by returning EINVAL from sendmsg. In that case, try again without setting the
     source address, since it will nearly alway be correct anyway.  IPv6 stinks. */
  if (sendmsg(fd, &msg, 0) == -1 && errno == EINVAL)
    {
      msg.msg_controllen = 0;
      sendmsg(fd, &msg, 0);
    }
}
          
static unsigned short search_servers(struct daemon *daemon, time_t now, struct all_addr **addrpp, 
				     unsigned short qtype, char *qdomain, int *type, char **domain)
			      
{
  /* If the query ends in the domain in one of our servers, set
     domain to point to that name. We find the largest match to allow both
     domain.org and sub.domain.org to exist. */
  
  unsigned int namelen = strlen(qdomain);
  unsigned int matchlen = 0;
  struct server *serv;
  unsigned short flags = 0;
  
  for (serv = daemon->servers; serv; serv=serv->next)
    /* domain matches take priority over NODOTS matches */
    if ((serv->flags & SERV_FOR_NODOTS) && *type != SERV_HAS_DOMAIN && !strchr(qdomain, '.'))
      {
	unsigned short sflag = serv->addr.sa.sa_family == AF_INET ? F_IPV4 : F_IPV6; 
	*type = SERV_FOR_NODOTS;
	if (serv->flags & SERV_NO_ADDR)
	  flags = F_NXDOMAIN;
	else if (serv->flags & SERV_LITERAL_ADDRESS) 
	  { 
	    if (sflag & qtype)
	      {
		flags = sflag;
		if (serv->addr.sa.sa_family == AF_INET) 
		  *addrpp = (struct all_addr *)&serv->addr.in.sin_addr;
#ifdef HAVE_IPV6
		else
		  *addrpp = (struct all_addr *)&serv->addr.in6.sin6_addr;
#endif 
	      }
	    else if (!flags)
	      flags = F_NOERR;
	  } 
      }
    else if (serv->flags & SERV_HAS_DOMAIN)
      {
	unsigned int domainlen = strlen(serv->domain);
	if (namelen >= domainlen &&
	    hostname_isequal(qdomain + namelen - domainlen, serv->domain) &&
	    domainlen >= matchlen)
	  {
	    unsigned short sflag = serv->addr.sa.sa_family == AF_INET ? F_IPV4 : F_IPV6;
	    *type = SERV_HAS_DOMAIN;
	    *domain = serv->domain;
	    matchlen = domainlen;
	    if (serv->flags & SERV_NO_ADDR)
	      flags = F_NXDOMAIN;
	    else if (serv->flags & SERV_LITERAL_ADDRESS)
	      {
		if ((sflag | F_QUERY ) & qtype)
		  {
		    flags = qtype;
		    if (serv->addr.sa.sa_family == AF_INET) 
		      *addrpp = (struct all_addr *)&serv->addr.in.sin_addr;
#ifdef HAVE_IPV6
		    else
		      *addrpp = (struct all_addr *)&serv->addr.in6.sin6_addr;
#endif
		  }
		else if (!flags)
		  flags = F_NOERR;
	      }
	  } 
      }

  if (flags & ~(F_NOERR | F_NXDOMAIN)) /* flags set here means a literal found */
    {
      if (flags & F_QUERY)
	log_query(F_CONFIG | F_FORWARD | F_NEG, qdomain, NULL, 0);
      else
	log_query(F_CONFIG | F_FORWARD | flags, qdomain, *addrpp, 0);
    }
  else if (qtype && (daemon->options & OPT_NODOTS_LOCAL) && !strchr(qdomain, '.'))
    flags = F_NXDOMAIN;
    
  if (flags == F_NXDOMAIN && check_for_local_domain(qdomain, now, daemon->mxnames))
    flags = F_NOERR;

  if (flags == F_NXDOMAIN || flags == F_NOERR)
    log_query(F_CONFIG | F_FORWARD | F_NEG | qtype | (flags & F_NXDOMAIN), qdomain, NULL, 0);

  return  flags;
}

/*  add start, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
#define HTONS_CHARS(n) (unsigned char)((n) >> 8), (unsigned char)(n)

static int add_device_id(struct daemon *daemon, HEADER *header, size_t plen, unsigned char *pheader,
 			 size_t pheader_len, struct frec *forward)
{
   const unsigned char clientid[11] = { HTONS_CHARS(4), HTONS_CHARS(15),
 				       'O', 'p', 'e', 'n', 'D', 'N', 'S' };
   const unsigned char fixed[11] = { 0, HTONS_CHARS(T_OPT),
 				    HTONS_CHARS(PACKETSZ),
 				    0, 0, 0, 0, HTONS_CHARS(0) };
   const int option_len = sizeof(clientid) + sizeof(daemon->device_id);
   unsigned char *p = (unsigned char *)header;
   unsigned short rdlen;
 
   if ((pheader == NULL && plen + sizeof(fixed) + option_len <= PACKETSZ)
       || (pheader != NULL && pheader + pheader_len == p + plen
 	  && plen + option_len <= PACKETSZ))
   {
       if (pheader == NULL)
 	   {
 	       pheader = p + plen;
 	       memcpy(p + plen, fixed, sizeof(fixed));
 	       plen += sizeof(fixed);
 	       header->arcount = htons(ntohs(header->arcount) + 1);
 	       /* Since the client didn't send a pseudoheader, it won't be
 	          expecting one in the response. */
 	       forward->discard_pseudoheader = 1;
#ifdef USE_SYSLOG
 	       if (daemon->options & OPT_LOG)
 	           my_syslog(LOG_DEBUG, "pseudoheader added");
#endif
 	   }
       /* Append a CLIENTID option to the pseudoheader. */
       memcpy(p + plen, clientid, sizeof(clientid));
       plen += sizeof(clientid);
       memcpy(p + plen, daemon->device_id,
 	     sizeof(daemon->device_id));
       plen += sizeof(daemon->device_id);
       /* Update the pseudoheader's RDLEN field. */
       p = pheader + 9;
       GETSHORT(rdlen, p);
       p = pheader + 9;
       PUTSHORT(rdlen + option_len, p);
#ifdef USE_SYSLOG
       if (daemon->options & OPT_LOG)
 	       my_syslog(LOG_DEBUG, "device ID added");
#endif
   }
   return plen;
}
#endif
/*  add end  , Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */

//#define QUERY_DEBUG 1 /* Michael */

/* returns new last_server */	
static void forward_query(struct daemon *daemon, int udpfd, union mysockaddr *udpaddr,
			  struct all_addr *dst_addr, unsigned int dst_iface,
			  HEADER *header, size_t plen, time_t now, struct frec *forward)
{
  //struct frec *forward;   /*  removed by EricHuang, 12/28/2007 */
  char *domain = NULL;
  int forwardall = 0, type = 0;
  struct all_addr *addrp = NULL;
  unsigned short flags = 0;
  unsigned short gotname = extract_request(header, (unsigned int)plen, daemon->namebuff, NULL);
  struct server *start = NULL;

    /* , add by MJ., for clarifying this issue. 2011.07.04 */
#ifdef QUERY_DEBUG
    printf("\n\n%s: sent from %s\n", __FUNCTION__, inet_ntoa(udpaddr->in.sin_addr));
    if(udpaddr->sa.sa_family == AF_INET)
        printf("A query from IPv4.\n");
    else
        printf("A query from IPv6.\n");
#endif
    /* , add-end by MJ., for clarifying this issue. 2011.07.04 */

  /*  add start, Tony W.Y. Wang, 12/05/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
  FILE *fp;
  char flag;
  char dnsquery_src_mac[20] = "";
  char device_id[32] = "";
  if((fp = fopen("/tmp/opendns.flag", "r")))
  {
      flag = fgetc(fp);
      fclose(fp);
  }
  daemon->have_device_id = 0;
  if(flag == '1')          /* Parental Control Enabled */
  {
      get_mac_from_arp(inet_ntoa(udpaddr->in.sin_addr), dnsquery_src_mac); /* Get MAC Address from ARP according to Soure IP */
      get_device_id(dnsquery_src_mac, device_id);
      daemon->have_device_id = 1;
      if(char_to_byte(device_id, daemon->device_id))
          return;
  }
#endif
  /*  add end  , Tony W.Y. Wang, 12/05/2008, @Parental Control OpenDNS */
#ifdef MULTIPLE_PPPOE
    int iToSession2, iIsSession2DNS;
  
    iToSession2 = IsDomainKeywordMatched(daemon->namebuff);
    /* if (mpoe == 1) */ /*  modified, zacker, 07/29/2008 */
    if (mpoe == 1 && forward)
        forward->forwardall = 1;    //forwardall = 1;       /*  modified by EricHuang, 01/02/2008 */

#endif 
/*  removed start by Jenny Zhao, 12/10/2008,@Russia_PPTP new spec*/
#if 0
    /*  added start, zacker, 07/29/2008,@Russia_PPTP */
    int iToDhcpDNS, iIsDhcpDNS;
    iToDhcpDNS = is_pptp_domain_matched(daemon->namebuff);
    /*  added end, zacker, 07/29/2008,@Russia_PPTP */
#endif
/*  removed end by Jenny Zhao, 12/10/2008,@Russia_PPTP new spec*/

  /* may be  recursion not speced or no servers available. */
    if (!header->rd || !daemon->servers)
        forward = NULL;
  /*  modified , Tony W.Y. Wang, 12/11/2008, @Parental Control OpenDNS to add device id in every DNS Query */
#ifdef OPENDNS_PARENTAL_CONTROL
    else if ( (forward || (forward = lookup_frec_by_sender(ntohs(header->id), udpaddr))) && (flag != '1')) /*  modified by EricHuang, 01/02/2008 */
#else
    else if ( forward || (forward = lookup_frec_by_sender(ntohs(header->id), udpaddr))) /*  modified by EricHuang, 01/02/2008 */
#endif
    {
        /* retry on existing query, send to all available servers  */
        domain = forward->sentto->domain;
        if (!(daemon->options & OPT_ORDER))
        {
            forward->forwardall = 1;  //forwardall = 1;       /*  modified by EricHuang, 01/02/2008 */
            daemon->last_server = NULL;
        }
        type = forward->sentto->flags & SERV_TYPE;
        if (!(start = forward->sentto->next))
            start = daemon->servers; /* at end of list, recycle */
        header->id = htons(forward->new_id);
    }
    else 
    {
        if (gotname)
	        flags = search_servers(daemon, now, &addrp, gotname, daemon->namebuff, &type, &domain);
      
        if (!flags && !(forward = get_new_frec(now)))
        	/* table full - server failure. */
        	flags = F_NEG;
      
        if (forward)
        {
          /*  add start, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
          size_t pheader_len;
          unsigned char *pheader;
          pheader = find_pseudoheader(header, plen, &pheader_len, NULL);
#endif
          /*  add end  , Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
          /*  moved start by EricHuang, 01/02/2008 */
          /*  added start Bob, 07/15/2011, check NULL pointer */
          if(udpaddr==NULL)
          {
          	return;
          }
          /*  added end Bob, 07/15/2011, check NULL pointer */
          forward->source = *udpaddr;
          forward->dest = *dst_addr;
          forward->iface = dst_iface;
          forward->new_id = get_id();
          forward->fd = udpfd;
          forward->orig_id = ntohs(header->id);
          forward->forwardall = 0;              /*  added by EricHuang, 01/02/2007 */
#ifdef OPENDNS_PARENTAL_CONTROL
          forward->discard_pseudoheader = 0;    /*  add, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#endif
          header->id = htons(forward->new_id);
          /*  moved start by EricHuang, 01/02/2008 */
            
          /* In strict_order mode, or when using domain specific servers
             always try servers in the order specified in resolv.conf,
             otherwise, use the one last known to work. */
          
          if (type != 0  || (daemon->options & OPT_ORDER))
            start = daemon->servers;
          else if (!(start = daemon->last_server))
          {
              start = daemon->servers;
              forward->forwardall = 1; //forwardall = 1;    /*  modified by EricHuang, 01/02/2007 */
          }
          /*  add start, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
          if (daemon->have_device_id)
              plen = add_device_id(daemon, header, plen, pheader, pheader_len, forward);
#endif
          /*  add end  , Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
          /*
          forward->source = *udpaddr;
          forward->dest = *dst_addr;
          forward->iface = dst_iface;
          forward->new_id = get_id();
          forward->fd = udpfd;
          forward->orig_id = ntohs(header->id);
          header->id = htons(forward->new_id);
          */
        }
    }

  /* check for send errors here (no route to host) 
     if we fail to send to all nameservers, send back an error
     packet straight away (helps modem users when offline)  */
  
  if (!flags && forward)
    {
      struct server *firstsentto = start;
      int forwarded = 0;
      /*  add start, Max Ding, 07/06/2011 */
      /* According to spec:
       * When the DNS query includes the type of AAAA or A6,
       * if there is a DNS server configured with an IPv6 address,
       * forward the query to the IPv6 DNS server. If there is
       * no DNS server configured with an IPv6 address or the
       * query fails to get an answer from the IPv6 DNS server (e.g. timeout, error...),
       * forward the query to the IPv4 DNS server configured. 
       *  
       * "query fail case" is to be implemented. 
       */
      unsigned short sflag = 0;
      int second_try = 0;
      /*  add end, Max Ding, 07/06/2011 */

      while (1)
	{ 
	  /* only send to servers dealing with our domain.
	     domain may be NULL, in which case server->domain 
	     must be NULL also. */
	  sflag = (start->addr.sa.sa_family == AF_INET) ? F_IPV4 : F_IPV6;/*  added by Max Ding, 07/06/2011 */  

      if (type == (start->flags & SERV_TYPE) &&
           ((gotname & F_QUERY) || (sflag & gotname)) &&/*  added by Max Ding, 07/06/2011 */
	      (type != SERV_HAS_DOMAIN || hostname_isequal(domain, start->domain)))
	    {
#ifdef MULTIPLE_PPPOE
	            char *pdnsServer;
	            struct sockaddr_in *p;
	            int iSendResult = 0;

#if 0                
                /* , add by MJ., 2011.07.05 */
                /* We don't forward a ipv4 query packets to DNS server with ipv6
                    * address, vice versa*/
                if(udpaddr)	/*  added Bob, 07/15/2011, check NULL pointer */
                {
                	if(!(start->flags & SERV_LITERAL_ADDRESS) && (start->addr.sa.sa_family != udpaddr->sa.sa_family) )
                	{
                    	//printf("=> don't forward this query.\n");
                    	goto try_next_server;
                	}
                }
                /* , add-end by MJ., 2011.07.05 */
#endif

	            p = (struct sockaddr_in *)&(start->addr.sa);
	            pdnsServer = inet_ntoa(p->sin_addr);
	            iIsSession2DNS = IsSession2DNS(p->sin_addr.s_addr);
	            
	            if(iIsSession2DNS == iToSession2)
	            {
	                iSendResult = sendto(start->sfd->fd, (char *)header, plen, 0, &start->addr.sa, sa_len(&start->addr));
	            }
		        if (!(start->flags & SERV_LITERAL_ADDRESS) && iSendResult != -1)
#else


/*  modified start by Jenny Zhao, 12/10/2008,@Russia_PPTP new spec*/
#if 0
            /* , add by MJ., 2011.07.05 */
            /* We don't forward a ipv4 query packets to DNS server with ipv6 address, vice versa*/
            if(!(start->flags & SERV_LITERAL_ADDRESS) && (start->addr.sa.sa_family != udpaddr->sa.sa_family) ){
                //printf("=> don't forward this query 2.\n");
                goto try_next_server;
            }
            /* , add-end by MJ., 2011.07.05 */
#endif                    
            /*  modified start, zacker, 07/29/2008,@Russia_PPTP */
            if (!(start->flags & SERV_LITERAL_ADDRESS) &&
                sendto(start->sfd->fd, (char *)header, plen, 0,
                &start->addr.sa,
                sa_len(&start->addr)) != -1)
#if 0
            char *pdnsServer;
            struct sockaddr_in *p;
            int iSendResult = 0;
            p = (struct sockaddr_in *)&(start->addr.sa);
            pdnsServer = inet_ntoa(p->sin_addr);
            iIsDhcpDNS = is_pptp_dhcp_dns(p->sin_addr.s_addr);

            if(iIsDhcpDNS == iToDhcpDNS)
            {
                iSendResult = sendto(start->sfd->fd, (char *)header, plen, 0, &start->addr.sa, sa_len(&start->addr));
            }
            if (!(start->flags & SERV_LITERAL_ADDRESS) && iSendResult != -1)
            /*  modified end, zacker, 07/29/2008,@Russia_PPTP */
#endif
/*  modified end by Jenny Zhao, 12/10/2008,@Russia_PPTP new spec*/
#endif /* MULTIPLE_PPPOE */
    		{

                /* , add by MJ., for clarifying this issue. */
#ifdef QUERY_DEBUG
                    char *pdnsServer;
                    struct sockaddr_in *p;

                    p = (struct sockaddr_in *)&(start->addr.sa);
                    pdnsServer = inet_ntoa(p->sin_addr);
                    printf("\nsend to %s\n", pdnsServer);
                    if (start->addr.sa.sa_family == AF_INET)
                        printf("DNS server is using an IPv4 address.\n");
                    else
                        printf("DNS server is using an IPv6 address.\n");
#endif
                /* , add-end by MJ., for clarifying this issue. */



    		  if (!gotname)
    		    strcpy(daemon->namebuff, "query");
    		  if (start->addr.sa.sa_family == AF_INET)
    		    log_query(F_SERVER | F_IPV4 | F_FORWARD, daemon->namebuff, 
    			      (struct all_addr *)&start->addr.in.sin_addr, 0); 
#ifdef HAVE_IPV6
    		  else
    		    log_query(F_SERVER | F_IPV6 | F_FORWARD, daemon->namebuff, 
    			      (struct all_addr *)&start->addr.in6.sin6_addr, 0);
#endif 
    		  forwarded = 1;
    		  forward->sentto = start;
    		  //if (!forwardall) 
    		  if (!forward->forwardall)     /*  modified by EricHuang, 01/02/2008 */
    		    break;
    		  
    		  forward->forwardall++;    /*  added by EricHuang, 01/02/2008 */
    		}/* Sento */
        } /* check if server is legal */

try_next_server:
	  if (!(start = start->next))
 	    start = daemon->servers;
	  
	  if (start == firstsentto)
      {
	      /*  add start, Max Ding, 07/06/2011 */
          /* According to Home Router Spec IPv6 part: 
           * if there is no DNS server configured with an IPv6 address,
           * forward the query to the IPv4 DNS server configured. 
           * if there is no DNS server configured with an IPv4 address, 
           * forward the query to the IPv6 DNS server configured. 
           */
          if ((second_try == 0) && (forwarded == 0)) 
          {
              if ((gotname & F_IPV4) && !(gotname & F_IPV6))
              {
                  gotname = F_IPV6;
                  second_try = 1;
                  continue;
              }
              else if ((gotname & F_IPV6) && !(gotname & F_IPV4)) 
              {
                  gotname = F_IPV4;
                  second_try = 1;
                  continue;
              }
          }
	      break;
       }
	}/* End of while(1) */
      
    if (forwarded)
        return;
      
      /* could not send on, prepare to return */ 
      header->id = htons(forward->orig_id);
      forward->new_id = 0; /* cancel */
    }	  

  /* could not send on, return empty answer or address if known for whole domain */
  plen = setup_reply(header, (unsigned int)plen, addrp, flags, daemon->local_ttl);
  if( udpaddr && dst_addr)
  {
      send_from(udpfd, daemon->options & OPT_NOWILD, (char *)header, plen, udpaddr, dst_addr, dst_iface);
  }
  
  return;
}

/*  modified, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
static int process_reply(struct daemon *daemon, HEADER *header, time_t now, 
			 union mysockaddr *serveraddr, unsigned int n, int discard_pseudoheader)
#else
static int process_reply(struct daemon *daemon, HEADER *header, time_t now, 
			 union mysockaddr *serveraddr, unsigned int n)
#endif
{
  unsigned char *pheader, *sizep;
  unsigned int plen;
   
  /* If upstream is advertising a larger UDP packet size
	 than we allow, trim it so that we don't get overlarge
	 requests for the client. */

  if ((pheader = find_pseudoheader(header, n, &plen, &sizep)))
    {
      unsigned short udpsz;
      unsigned char *psave = sizep;
      
      GETSHORT(udpsz, sizep);
      if (udpsz > daemon->edns_pktsz)
	      PUTSHORT(daemon->edns_pktsz, psave);
	  /*  add start, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
	  if (discard_pseudoheader
	  && pheader + plen == (unsigned char *)header + n)
 	  {
 	      header->arcount = htons(ntohs(header->arcount) - 1);
 	      n -= plen;
 	      pheader = NULL;
 	  }
#endif
 	  /*  add end  , Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
  }

  /* Complain loudly if the upstream server is non-recursive. */
  if (!header->ra && header->rcode == NOERROR && ntohs(header->ancount) == 0)
    {
      char addrbuff[ADDRSTRLEN];
#ifdef HAVE_IPV6
      if (serveraddr->sa.sa_family == AF_INET)
	inet_ntop(AF_INET, &serveraddr->in.sin_addr, addrbuff, ADDRSTRLEN);
      else if (serveraddr->sa.sa_family == AF_INET6)
	inet_ntop(AF_INET6, &serveraddr->in6.sin6_addr, addrbuff, ADDRSTRLEN);
#else
      strcpy(addrbuff, inet_ntoa(serveraddr->in.sin_addr));
#endif
#ifdef USE_SYSLOG /*  wklin added, 08/13/2007 */
      syslog(LOG_WARNING, "nameserver %s refused to do a recursive query", addrbuff);
#endif       
      return 0;
    }
  
  if (header->opcode != QUERY || (header->rcode != NOERROR && header->rcode != NXDOMAIN))
    return n;
  
  if (header->rcode == NOERROR && ntohs(header->ancount) != 0)
    {
      if (!(daemon->bogus_addr && 
	    check_for_bogus_wildcard(header, n, daemon->namebuff, daemon->bogus_addr, now)))
	extract_addresses(header, n, daemon->namebuff, now, daemon->doctors);
    }
  else
    {
      unsigned short flags = F_NEG;
      int munged = 0;

      if (header->rcode == NXDOMAIN)
	{
	  /* if we forwarded a query for a locally known name (because it was for 
	     an unknown type) and the answer is NXDOMAIN, convert that to NODATA,
	     since we know that the domain exists, even if upstream doesn't */
	  if (extract_request(header, n, daemon->namebuff, NULL) &&
	      check_for_local_domain(daemon->namebuff, now, daemon->mxnames))
	    {
	      munged = 1;
	      header->rcode = NOERROR;
	    }
	  else
	    flags |= F_NXDOMAIN;
	}
      
      if (!(daemon->options & OPT_NO_NEG))
	extract_neg_addrs(header, n, daemon->namebuff, now, flags);
	  
      /* do this after extract_neg_addrs. Ensure NODATA reply and remove
	 nameserver info. */
      if (munged)
	{
	  header->ancount = htons(0);
	  header->nscount = htons(0);
	  header->arcount = htons(0);
	}
    }

  /* the bogus-nxdomain stuff, doctor and NXDOMAIN->NODATA munging can all elide
     sections of the packet. Find the new length here and put back pseudoheader
     if it was removed. */
  return resize_packet(header, n, pheader, plen);
}

/* sets new last_server */
void reply_query(struct serverfd *sfd, struct daemon *daemon, time_t now)
{
    /* packet from peer server, extract data for cache, and send to
     original requester */
    struct frec *forward;
    HEADER *header;
    union mysockaddr serveraddr;
    socklen_t addrlen = sizeof(serveraddr);
    int n = recvfrom(sfd->fd, daemon->packet, daemon->edns_pktsz, 0, &serveraddr.sa, &addrlen);
    size_t nn;

    /*  add start, Tony W.Y. Wang, 07/09/2010, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
    FILE *fp;
    char flag;
    if((fp = fopen("/tmp/opendns.flag", "r")))
    {
        flag = fgetc(fp);
        fclose(fp);
    }
#endif
    /*  add start, Tony W.Y. Wang, 07/09/2010, @Parental Control OpenDNS */
    /* Determine the address of the server replying  so that we can mark that as good */
    serveraddr.sa.sa_family = sfd->source_addr.sa.sa_family;
#ifdef HAVE_IPV6
    if (serveraddr.sa.sa_family == AF_INET6)
    serveraddr.in6.sin6_flowinfo = htonl(0);
#endif
  
    header = (HEADER *)daemon->packet;
    forward = lookup_frec(ntohs(header->id)); /*  added by EricHuang, 01/02/2008 */

    if (n >= (int)sizeof(HEADER) && header->qr && forward)
    {
        /*  added start by EricHuang, 01/02/2008 */
        struct server *server = forward->sentto;
        /*  add start, Tony W.Y. Wang, 07/09/2010 */
#ifdef OPENDNS_PARENTAL_CONTROL
        if ((header->rcode == SERVFAIL || header->rcode == REFUSED) && forward->forwardall == 0 && (flag != '1'))
#else
        if ((header->rcode == SERVFAIL || header->rcode == REFUSED) && forward->forwardall == 0)
#endif
        /*  add end, Tony W.Y. Wang, 07/09/2010 */
        /* for broken servers, attempt to send to another one. */
        {
            unsigned char *pheader;
            size_t plen;
            
            /* recreate query from reply */
            pheader = find_pseudoheader(header, (size_t)n, &plen, NULL);
            header->ancount = htons(0);
            header->nscount = htons(0);
            header->arcount = htons(0);
            if ((nn = resize_packet(header, (size_t)n, pheader, plen)))
            {
               forward->forwardall = 1;
               header->qr = 0;
               header->tc = 0;
               forward_query(daemon, -1, NULL, NULL, 0, header, nn, now, forward);
               return;
            }
        }
        /*  added end by EricHuang, 01/02/2008 */
     
        
        /* find good server by address if possible, otherwise assume the last one we sent to */ 
        if ((forward->sentto->flags & SERV_TYPE) == 0)
        {
            
            if (header->rcode == SERVFAIL || header->rcode == REFUSED)
                server = NULL;
            else
            {
                struct server *last_server;
                /* find good server by address if possible, otherwise assume the last one we sent to */ 
                for (last_server = daemon->servers; last_server; last_server = last_server->next)
                    if (!(last_server->flags & (SERV_LITERAL_ADDRESS | SERV_HAS_DOMAIN | SERV_FOR_NODOTS | SERV_NO_ADDR)) &&
                    sockaddr_isequal(&last_server->addr, &serveraddr))
                    {
                        server = last_server;
                        break;
                    }
            } 
            daemon->last_server = server;

	    }


        /* If the answer is an error, keep the forward record in place in case
        we get a good reply from another server. Kill it when we've
        had replies from all to avoid filling the forwarding table when
        everything is broken */
        if (forward->forwardall == 0 || --forward->forwardall == 1 || 
            (header->rcode != REFUSED && header->rcode != SERVFAIL))
        {
#ifdef OPENDNS_PARENTAL_CONTROL
            if ((nn = process_reply(daemon, header, now, &server->addr, (size_t)n, 
                forward->discard_pseudoheader))) /*  modified, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#else
            if ((nn = process_reply(daemon, header, now,  &server->addr, (size_t)n)))
#endif
            {
            /*  wklin added start, 09/03/2007 @mpoe */
#ifdef MULTIPLE_PPPOE
                private_domain_check(header, n, daemon->namebuff);
#endif /* MULTIPLE_PPPOE */
            /*  wklin added end, 09/03/2007 */
            
                header->id = htons(forward->orig_id);
                header->ra = 1; /* recursion if available */
                send_from(forward->fd, daemon->options & OPT_NOWILD, daemon->packet, nn, 
                &forward->source, &forward->dest, forward->iface);
            }
            forward->new_id = 0; /* cancel */
        }
    }
}

void receive_query(struct listener *listen, struct daemon *daemon, time_t now)
{
  HEADER *header = (HEADER *)daemon->packet;
  union mysockaddr source_addr;
  unsigned short type;
  struct iname *tmp;
  struct all_addr dst_addr;
  int check_dst = !(daemon->options & OPT_NOWILD);
  int m, n, if_index = 0;
  struct iovec iov[1];
  struct msghdr msg;
  struct cmsghdr *cmptr;
  union {
    struct cmsghdr align; /* this ensures alignment */
#ifdef HAVE_IPV6
    char control6[CMSG_SPACE(sizeof(struct in6_pktinfo))];
#endif
#if defined(IP_PKTINFO)
    char control[CMSG_SPACE(sizeof(struct in_pktinfo))];
#elif defined(IP_RECVDSTADDR)
    char control[CMSG_SPACE(sizeof(struct in_addr)) +
		 CMSG_SPACE(sizeof(struct sockaddr_dl))];
#endif
  } control_u;
  
  iov[0].iov_base = daemon->packet;
  iov[0].iov_len = daemon->edns_pktsz;
    
  msg.msg_control = control_u.control;
  msg.msg_controllen = sizeof(control_u);
  msg.msg_flags = 0;
  msg.msg_name = &source_addr;
  msg.msg_namelen = sizeof(source_addr);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  
  if ((n = recvmsg(listen->fd, &msg, 0)) == -1)
    return;

  /* wklin modified start, 01/24/2007 */
  /* Before getting dns IPs from ISP, dnsmasq will reject the
   * queries from clients, and client will thus think the dns queries 
   * fails. When DoD is on, the first Internet access will always fail.
   * Modified the code here after the packet is consumed, so that the 
   * dnsmasq won't reject the queries.
   */
  if (!daemon->servers)
    return;
  /* wklin modified end, 01/24/2007 */
    
  source_addr.sa.sa_family = listen->family;
#ifdef HAVE_IPV6
  if (listen->family == AF_INET6)
    {
      check_dst = 1;
      source_addr.in6.sin6_flowinfo = htonl(0);
    }
#endif
  
  if (check_dst && msg.msg_controllen < sizeof(struct cmsghdr))
    return;

#if defined(IP_PKTINFO)
  if (check_dst && listen->family == AF_INET)
    for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr))
      if (cmptr->cmsg_level == SOL_IP && cmptr->cmsg_type == IP_PKTINFO)
	{
	  dst_addr.addr.addr4 = ((struct in_pktinfo *)CMSG_DATA(cmptr))->ipi_spec_dst;
	  if_index = ((struct in_pktinfo *)CMSG_DATA(cmptr))->ipi_ifindex;
	}
#elif defined(IP_RECVDSTADDR) && defined(IP_RECVIF)
  if (check_dst && listen->family == AF_INET)
    {
      for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr))
	if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVDSTADDR)
	  dst_addr.addr.addr4 = *((struct in_addr *)CMSG_DATA(cmptr));
	else if (cmptr->cmsg_level == IPPROTO_IP && cmptr->cmsg_type == IP_RECVIF)
	  if_index = ((struct sockaddr_dl *)CMSG_DATA(cmptr))->sdl_index;
    }
#endif

#ifdef HAVE_IPV6
  if (listen->family == AF_INET6)
    {
      for (cmptr = CMSG_FIRSTHDR(&msg); cmptr; cmptr = CMSG_NXTHDR(&msg, cmptr))
	if (cmptr->cmsg_level == IPV6_LEVEL && cmptr->cmsg_type == IPV6_PKTINFO)
	  {
	    dst_addr.addr.addr6 = ((struct in6_pktinfo *)CMSG_DATA(cmptr))->ipi6_addr;
	    if_index =((struct in6_pktinfo *)CMSG_DATA(cmptr))->ipi6_ifindex;
	  }
    }
#endif
  
  if (n < (int)sizeof(HEADER) || header->qr)
    return;
  
  /* enforce available interface configuration */
  if (check_dst)
    {
      struct ifreq ifr;

      if (if_index == 0)
	return;
      
      if (daemon->if_except || daemon->if_names)
	{
#ifdef SIOCGIFNAME
	  ifr.ifr_ifindex = if_index;
	  if (ioctl(listen->fd, SIOCGIFNAME, &ifr) == -1)
	    return;
#else
	  if (!if_indextoname(if_index, ifr.ifr_name))
	    return;
#endif
	}

      for (tmp = daemon->if_except; tmp; tmp = tmp->next)
	if (tmp->name && (strcmp(tmp->name, ifr.ifr_name) == 0))
	  return;
      
      if (daemon->if_names || daemon->if_addrs)
	{
	  for (tmp = daemon->if_names; tmp; tmp = tmp->next)
	    if (tmp->name && (strcmp(tmp->name, ifr.ifr_name) == 0))
	      break;
	  if (!tmp)
	    for (tmp = daemon->if_addrs; tmp; tmp = tmp->next)
	      if (tmp->addr.sa.sa_family == listen->family)
		{
		  if (tmp->addr.sa.sa_family == AF_INET &&
		      tmp->addr.in.sin_addr.s_addr == dst_addr.addr.addr4.s_addr)
		    break;
#ifdef HAVE_IPV6
		  else if (tmp->addr.sa.sa_family == AF_INET6 &&
			   memcmp(&tmp->addr.in6.sin6_addr, 
				  &dst_addr.addr.addr6, 
				  sizeof(struct in6_addr)) == 0)
		    break;
#endif
		}
	  if (!tmp)
	    return; 
	}
    }
  
  if (extract_request(header, (unsigned int)n, daemon->namebuff, &type))
    {
      if (listen->family == AF_INET) 
	log_query(F_QUERY | F_IPV4 | F_FORWARD, daemon->namebuff, 
		  (struct all_addr *)&source_addr.in.sin_addr, type);
#ifdef HAVE_IPV6
      else
	log_query(F_QUERY | F_IPV6 | F_FORWARD, daemon->namebuff, 
		  (struct all_addr *)&source_addr.in6.sin6_addr, type);
#endif
    }

  m = answer_request (header, ((char *) header) + PACKETSZ, (unsigned int)n, daemon, now);
  if (m >= 1)
    send_from(listen->fd, daemon->options & OPT_NOWILD, (char *)header, m, &source_addr, &dst_addr, if_index);
  else
    forward_query(daemon, listen->fd, &source_addr, &dst_addr, if_index,
		  header, n, now, NULL); /*  modified by EricHuang, 01/02/2008 */
}

static int read_write(int fd, char *packet, int size, int rw)
{
  int n, done;
  
  for (done = 0; done < size; done += n)
    {
    retry:
      if (rw)
	n = read(fd, &packet[done], (size_t)(size - done));
      else
	n = write(fd, &packet[done], (size_t)(size - done));

      if (n == 0)
	return 0;
      else if (n == -1)
	{
	  if (errno == EINTR)
	    goto retry;
	  else if (errno == EAGAIN)
	    {
	      struct timespec waiter;
	      waiter.tv_sec = 0;
	      waiter.tv_nsec = 10000;
	      nanosleep(&waiter, NULL);
	      goto retry;
	    }
	  else
	    return 0;
	}
    }
  return 1;
}
  
/* The daemon forks before calling this: it should deal with one connection,
   blocking as neccessary, and then return. Note, need to be a bit careful
   about resources for debug mode, when the fork is suppressed: that's
   done by the caller. */
char *tcp_request(struct daemon *daemon, int confd, time_t now)
{
  int size = 0, m;
  unsigned short qtype, gotname;
  unsigned char c1, c2;
  /* Max TCP packet + slop */
  char *packet = malloc(65536 + MAXDNAME + RRFIXEDSZ);
  HEADER *header;
  struct server *last_server;
  
  while (1)
    {
      if (!packet ||
	  !read_write(confd, &c1, 1, 1) || !read_write(confd, &c2, 1, 1) ||
	  !(size = c1 << 8 | c2) ||
	  !read_write(confd, packet, size, 1))
       	return packet; 
  
      if (size < (int)sizeof(HEADER))
	continue;
      
      header = (HEADER *)packet;
      
      if ((gotname = extract_request(header, (unsigned int)size, daemon->namebuff, &qtype)))
	{
	  union mysockaddr peer_addr;
	  socklen_t peer_len = sizeof(union mysockaddr);
	  
	  if (getpeername(confd, (struct sockaddr *)&peer_addr, &peer_len) != -1)
	    {
	      if (peer_addr.sa.sa_family == AF_INET) 
		log_query(F_QUERY | F_IPV4 | F_FORWARD, daemon->namebuff, 
			  (struct all_addr *)&peer_addr.in.sin_addr, qtype);
#ifdef HAVE_IPV6
	      else
		log_query(F_QUERY | F_IPV6 | F_FORWARD, daemon->namebuff, 
			  (struct all_addr *)&peer_addr.in6.sin6_addr, qtype);
#endif
	    }
	}
      
      /* m > 0 if answered from cache */
      m = answer_request(header, ((char *) header) + 65536, (unsigned int)size, daemon, now);
      
      if (m == 0)
	{
	  unsigned short flags = 0;
	  struct all_addr *addrp = NULL;
	  int type = 0;
	  char *domain = NULL;
	  
	  if (gotname)
	    flags = search_servers(daemon, now, &addrp, gotname, daemon->namebuff, &type, &domain);
	  
	  if (type != 0  || (daemon->options & OPT_ORDER) || !daemon->last_server)
	    last_server = daemon->servers;
	  else
	    last_server = daemon->last_server;
      
	  if (!flags && last_server)
	    {
	      struct server *firstsendto = NULL;
	      
	      /* Loop round available servers until we succeed in connecting to one.
	         Note that this code subtley ensures that consecutive queries on this connection
	         which can go to the same server, do so. */
	      while (1) 
		{
		  if (!firstsendto)
		    firstsendto = last_server;
		  else
		    {
		      if (!(last_server = last_server->next))
			last_server = daemon->servers;
		      
		      if (last_server == firstsendto)
			break;
		    }
	      
		  /* server for wrong domain */
		  if (type != (last_server->flags & SERV_TYPE) ||
		      (type == SERV_HAS_DOMAIN && !hostname_isequal(domain, last_server->domain)))
		    continue;
		  
		  if ((last_server->tcpfd == -1) &&
		      (last_server->tcpfd = socket(last_server->addr.sa.sa_family, SOCK_STREAM, 0)) != -1 &&
		      connect(last_server->tcpfd, &last_server->addr.sa, sa_len(&last_server->addr)) == -1)
		    {
		      close(last_server->tcpfd);
		      last_server->tcpfd = -1;
		    }
		  
		  if (last_server->tcpfd == -1)	
		    continue;
		  
		  c1 = size >> 8;
		  c2 = size;
		  
		  if (!read_write(last_server->tcpfd, &c1, 1, 0) ||
		      !read_write(last_server->tcpfd, &c2, 1, 0) ||
		      !read_write(last_server->tcpfd, packet, size, 0) ||
		      !read_write(last_server->tcpfd, &c1, 1, 1) ||
		      !read_write(last_server->tcpfd, &c2, 1, 1))
		    {
		      close(last_server->tcpfd);
		      last_server->tcpfd = -1;
		      continue;
		    } 
	      
		  m = (c1 << 8) | c2;
		  if (!read_write(last_server->tcpfd, packet, m, 1))
		    return packet;
		  
		  if (!gotname)
		    strcpy(daemon->namebuff, "query");
		  if (last_server->addr.sa.sa_family == AF_INET)
		    log_query(F_SERVER | F_IPV4 | F_FORWARD, daemon->namebuff, 
			      (struct all_addr *)&last_server->addr.in.sin_addr, 0); 
#ifdef HAVE_IPV6
		  else
		    log_query(F_SERVER | F_IPV6 | F_FORWARD, daemon->namebuff, 
			      (struct all_addr *)&last_server->addr.in6.sin6_addr, 0);
#endif 
		  
		  /* There's no point in updating the cache, since this process will exit and
		     lose the information after one query. We make this call for the alias and 
		     bogus-nxdomain side-effects. */
		  /*  modified, Tony W.Y. Wang, 12/02/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
		  m = process_reply(daemon, header, now, &last_server->addr, (unsigned int)m, 0);
#else
		  m = process_reply(daemon, header, now, &last_server->addr, (unsigned int)m);
#endif
		  break;
		}
	    }
	  
	  /* In case of local answer or no connections made. */
	  if (m == 0)
	    m = setup_reply(header, (unsigned int)size, addrp, flags, daemon->local_ttl);
	}
      
      c1 = m>>8;
      c2 = m;
      if (!read_write(confd, &c1, 1, 0) ||
	  !read_write(confd, &c2, 1, 0) || 
	  !read_write(confd, packet, m, 0))
	return packet;
    }
}

static struct frec *get_new_frec(time_t now)
{
  struct frec *f = frec_list, *oldest = NULL;
  time_t oldtime = now;
  int count = 0;
  static time_t warntime = 0;

  while (f)
    {
      if (f->new_id == 0)
	{
	  f->time = now;
	  return f;
	}

      if (difftime(f->time, oldtime) <= 0)
	{
	  oldtime = f->time;
	  oldest = f;
	}

      count++;
      f = f->next;
    }
  
  /* can't find empty one, use oldest if there is one
     and it's older than timeout */
  if (oldest && difftime(now, oldtime)  > TIMEOUT)
    { 
      oldest->time = now;
      return oldest;
    }
  
  if (count > FTABSIZ)
    { /* limit logging rate so syslog isn't DOSed either */
      if (!warntime || difftime(now, warntime) > LOGRATE)
	{
	  warntime = now;
#ifdef USE_SYSLOG /*  wklin added, 08/13/2007 */
	  syslog(LOG_WARNING, "forwarding table overflow: check for server loops.");
#endif
	}
      return NULL;
    }

  if ((f = (struct frec *)malloc(sizeof(struct frec))))
    {
      f->next = frec_list;
      f->time = now;
      frec_list = f;
    }
  return f; /* OK if malloc fails and this is NULL */
}
 
static struct frec *lookup_frec(unsigned short id)
{
  struct frec *f;

  for(f = frec_list; f; f = f->next)
    if (f->new_id == id)
      return f;
      
  return NULL;
}

static struct frec *lookup_frec_by_sender(unsigned short id,
					  union mysockaddr *addr)
{
    struct frec *f;
    
    if( addr )
    {
  	    for(f = frec_list; f; f = f->next)
    	    if (f->new_id &&
			    f->orig_id == id && 
			    sockaddr_isequal(&f->source, addr))
          return f;
    }
   
  return NULL;
}


/* return unique random ids between 1 and 65535 */
static unsigned short get_id(void)
{
  unsigned short ret = 0;

  while (ret == 0)
    {
      ret = rand16();
      
      /* scrap ids already in use */
      if ((ret != 0) && lookup_frec(ret))
	ret = 0;
    }

  return ret;
}

/*  add start, Tony W.Y. Wang, 12/05/2008, @Parental Control OpenDNS */
#ifdef OPENDNS_PARENTAL_CONTROL
#define PROC_ARP_FILE    "/proc/net/arp"
int get_mac_from_arp(char *dnsquery_src_ip, char *src_mac)
{
    FILE *fp;
    char buf[512];
    char buffer[64];
    int  i = 0, index = 0;
    int exist_num = 0;
    char *p_str;
    char space=' ';
    char *s_str=NULL;
    if (!(fp = fopen(PROC_ARP_FILE, "r"))) {
        perror(PROC_ARP_FILE);
        return 0;
    }
    fgets(buf, sizeof(buf), fp); /* ignore the first line */
    while (fgets(buf, sizeof(buf), fp)) {      /* get all the lines */
	s_str=strchr(buf,space);
	if(strncmp(buf, dnsquery_src_ip,s_str-buf) == 0)
    {
        p_str = strstr (buf, dnsquery_src_ip); /* check whether the src_ip exist in the line */
        if(p_str)
        {
            p_str = p_str + 41;             
            strncpy(buf,p_str,17);             /* get MAC 00:11:22:33:44:55 */  
            buf[17] = '\0';
            for(i=0; i<17; i++)                /* transfor MAC to 001122334455 */
            {
                if(buf[i] != ':')
                {
                    buffer[index] = buf[i];
                    index++;
                }           
            }
            buffer[index] = '\0';
            strcpy(src_mac, buffer);
		    fclose(fp);
		    return 0;
        }
		}
    }
    fclose(fp);
    return 1;
}

int char_to_byte(char string_id[], unsigned char byte_id[])
{
    int i = 0;
    int tmp = 0;
    for (i=0; i<16; i+=2)
    {
        if(string_id[i] >= '0' && string_id[i] <= '9')
            tmp = string_id[i] - '0';
        else if(string_id[i] >= 'A' && string_id[i] <= 'F')
            tmp = string_id[i] - 'A' + 10;        
        if(string_id[i+1] >= '0' && string_id[i+1] <= '9')
            tmp = tmp*16 + (string_id[i+1] - '0');
        else if(string_id[i+1] >= 'A' && string_id[i+1] <= 'F')
            tmp = tmp*16 + (string_id[i+1] - 'A' + 10); 
        byte_id[i/2] = (unsigned char)tmp;
    }
    return 0;
}

static void get_device_id(char src_mac[], char id[])
{
    FILE *fp;
    char opendns_tbl[2048] = "";
    int is_found = 0; 
    char *p_str = NULL;
  
    if((fp = fopen("/tmp/opendns.tbl", "r")))
    {
        while (fgets(opendns_tbl, sizeof(opendns_tbl), fp))
        {
            p_str = strstr (opendns_tbl, "DEFAULT");
            if(p_str)
            {
                is_found = 1;
                p_str = p_str + strlen("DEFAULT") + 1;
                strncpy(id, p_str, 16);
                id[16] = '\0';
            }
            //p_str = strstr (opendns_tbl, src_mac);
            p_str = strcasestr (opendns_tbl, src_mac);
            if(p_str)
            {
                is_found = 1;
                p_str = p_str + strlen(src_mac) + 1;
                strncpy(id, p_str, 16);
                id[16] = '\0';
                break;
            }
        }
        fclose(fp);
    }
    if(!is_found)
    {
        strcpy(id, "0000111111111111");
    }
}
#endif
/*  add end  , Tony W.Y. Wang, 12/05/2008, @Parental Control OpenDNS */


