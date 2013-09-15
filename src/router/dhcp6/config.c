/*	$Id: config.c,v 1.1.1.1 2006/12/04 00:45:20 Exp $	*/
/*	ported from KAME: config.c,v 1.21 2002/09/24 14:20:49 itojun Exp */

/*
 * Copyright (C) 2002 WIDE Project.
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
#include <sys/types.h>
#include <sys/socket.h>

#include <net/if.h>

#include <netinet/in.h>

#include <syslog.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ifaddrs.h>

#include "queue.h"
#include "dhcp6.h"
#include "config.h"
#include "common.h"

extern int errno;

static struct dhcp6_ifconf *dhcp6_ifconflist;
static struct host_conf *host_conflist0, *host_conflist;
static struct dhcp6_list dnslist0; 

enum { DHCPOPTCODE_SEND, DHCPOPTCODE_REQUEST, DHCPOPTCODE_ALLOW };

static int add_options __P((int, struct dhcp6_ifconf *, struct cf_list *));
static int add_address __P((struct dhcp6_list *, struct dhcp6_addr *));
static int add_option  __P((struct dhcp6_option_list *, struct cf_list *));
static int clear_option_list  __P((struct dhcp6_option_list *));

static void clear_ifconf __P((struct dhcp6_ifconf *));
static void clear_hostconf __P((struct host_conf *));

/*  added start pling 09/17/2010 */
static char user_class[MAX_USER_CLASS_LEN];
int set_dhcpc_user_class(char *str)
{
	strcpy(user_class, str);
    dprintf(LOG_INFO, "Set User-class [%s]", user_class);
    return 0;
}
/*  added end pling 09/17/2010 */

/*  added start pling 10/07/2010 */
/* For testing purposes */
extern u_int32_t xid_solicit;
extern u_int32_t xid_request;
extern u_int32_t duid_time;
/*  added end pling 10/07/2010 */

int
configure_interface(const struct cf_namelist *iflist)
{
	const struct cf_namelist *ifp;
	struct dhcp6_ifconf *ifc;

	for (ifp = iflist; ifp; ifp = ifp->next) {
		struct cf_list *cfl;

		if ((ifc = malloc(sizeof(*ifc))) == NULL) {
			dprintf(LOG_ERR, "%s"
				"memory allocation for %s failed", FNAME,
				ifp->name);
			goto bad;
		}
		memset(ifc, 0, sizeof(*ifc));
		ifc->next = dhcp6_ifconflist;
		dhcp6_ifconflist = ifc;

		if ((ifc->ifname = strdup(ifp->name)) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to copy ifname", FNAME);
			goto bad;
		}

		ifc->server_pref = DH6OPT_PREF_UNDEF;
		TAILQ_INIT(&ifc->reqopt_list);
		TAILQ_INIT(&ifc->addr_list);
		TAILQ_INIT(&ifc->option_list);

		for (cfl = ifp->params; cfl; cfl = cfl->next) {
			switch(cfl->type) {
			case DECL_REQUEST:
				if (dhcp6_mode != DHCP6_MODE_CLIENT) {
					dprintf(LOG_INFO, "%s" "%s:%d "
						"client-only configuration",
						FNAME, configfilename,
						cfl->line);
					goto bad;
				}
				if (add_options(DHCPOPTCODE_REQUEST,
						ifc, cfl->list)) {
					goto bad;
				}
				break;
			case DECL_SEND:
				if (add_options(DHCPOPTCODE_SEND,
						ifc, cfl->list)) {
					goto bad;
				}
				break;
			case DECL_ALLOW:
				if (add_options(DHCPOPTCODE_ALLOW,
						ifc, cfl->list)) {
					goto bad;
				}
				break;
			case DECL_INFO_ONLY:
				if (dhcp6_mode == DHCP6_MODE_CLIENT)
                {
					ifc->send_flags |= DHCIFF_INFO_ONLY;
                    /*  added start pling 09/23/2010 */
                    set_dhcp6c_flags(DHCIFF_INFO_ONLY);
                    /*  added end pling 09/23/2010 */
                }
				break;
			case DECL_TEMP_ADDR:
				if (dhcp6_mode == DHCP6_MODE_CLIENT)
					ifc->send_flags |= DHCIFF_TEMP_ADDRS;
				break;
			case DECL_PREFERENCE:
				if (dhcp6_mode != DHCP6_MODE_SERVER) {
					dprintf(LOG_INFO, "%s" "%s:%d "
						"server-only configuration",
						FNAME, configfilename,
						cfl->line);
					goto bad;
				}
				ifc->server_pref = (int)cfl->num;
				if (ifc->server_pref < 0 ||
				    ifc->server_pref > 255) {
					dprintf(LOG_INFO, "%s" "%s:%d "
						"bad value: %d", FNAME,
						configfilename, cfl->line,
						ifc->server_pref);
					goto bad;
				}
				break;
			case DECL_IAID:
				if (ifc->iaidinfo.iaid) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated IAID for %s",
						FNAME, configfilename,
						cfl->line, ifc->ifname);
					goto bad;
				} else
					ifc->iaidinfo.iaid = (u_int32_t)cfl->num;
				break;
			case DECL_RENEWTIME:
				if (ifc->iaidinfo.renewtime) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated renewtime for %s",
						FNAME, configfilename,
						cfl->line, ifc->ifname);
					goto bad;
				} else
					ifc->iaidinfo.renewtime = (u_int32_t)cfl->num;
				break;
			case DECL_REBINDTIME:
				if (ifc->iaidinfo.iaid) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated rebindtime for %s",
						FNAME, configfilename,
						cfl->line, ifc->ifname);
					goto bad;
				} else 
					ifc->iaidinfo.rebindtime = (u_int32_t)cfl->num;
				break;
			case DECL_ADDRESS:
				if (add_address(&ifc->addr_list, cfl->ptr)) {
					dprintf(LOG_ERR, "%s" "failed "
						"to configure ipv6address for %s",
						FNAME, ifc->ifname);
					goto bad;
				}
				break;
			case DECL_PREFIX_REQ:
				/* XX: ToDo */
				break;
			case DECL_PREFIX_INFO:
				break;
			case DECL_PREFIX_DELEGATION_INTERFACE:
			        if (add_option(&ifc->option_list, cfl)){
					dprintf(LOG_ERR, "%s failed to configure prefix-delegation-interface for %s",
						FNAME, ifc->ifname);
					goto bad;
				}
				break;
			/*  added start pling 08/26/2009 */
			/* Add flag to send dhcpv6 solicit only for DHCP client.
			 * Used by IPv6 auto detection.
			 */
			case DECL_SOLICIT_ONLY:
				if (dhcp6_mode == DHCP6_MODE_CLIENT)
				{
					dprintf(LOG_ERR, "Send DHCPC solicit only!");
					ifc->send_flags |= DHCIFF_SOLICIT_ONLY;
				}
				break;
			/*  added end pling 08/26/2009 */
			/*  added start pling 09/07/2010 */
			/* Support user-class for DHCP client */
			case DECL_USER_CLASS:
				if (dhcp6_mode == DHCP6_MODE_CLIENT)
				{
				    dprintf(LOG_ERR, "%s assign user_class='%s'", FNAME, user_class);
					if (strlen(user_class))
						strcpy(ifc->user_class, user_class);
					else
						ifc->user_class[0] = '\0';
				}
				break;
			/*  added end pling 09/07/2010 */
            /*  added start pling 09/21/2010 */
            /* For DHCPv6 readylogo, need to send IANA and IAPD separately.
             */
            case DECL_IANA_ONLY:
                if (dhcp6_mode == DHCP6_MODE_CLIENT)
                {
                    dprintf(LOG_ERR, "Accept IANA only!");
                    set_dhcp6c_flags(DHCIFF_IANA_ONLY);
                }
                break;
            case DECL_IAPD_ONLY:
                if (dhcp6_mode == DHCP6_MODE_CLIENT)
                {
                    dprintf(LOG_ERR, "Accept IAPD only!");
                    set_dhcp6c_flags(DHCIFF_IAPD_ONLY);
                }
                break;
            /*  added end pling 09/21/2010 */
            /*  added start pling 10/07/2010 */
            /* For Testing purposes */
            case DECL_XID_SOL:
                xid_solicit =  (u_int32_t)cfl->num;
                break;
            case DECL_XID_REQ:
                xid_request = (u_int32_t)cfl->num;
                break;
            case DECL_DUID_TIME:
                duid_time = (u_int32_t)cfl->num;
                break;
            /*  added end pling 10/07/2010 */
			default:
				dprintf(LOG_ERR, "%s" "%s:%d "
					"invalid interface configuration",
					FNAME, configfilename, cfl->line);
				goto bad;
			}
		}
	}
	
	return (0);

  bad:
	clear_ifconf(dhcp6_ifconflist);
	dhcp6_ifconflist = NULL;
	return (-1);
}

int
configure_host(const struct cf_namelist *hostlist)
{
	const struct cf_namelist *host;
	struct host_conf *hconf;
	
	for (host = hostlist; host; host = host->next) {
		struct cf_list *cfl;

		if ((hconf = malloc(sizeof(*hconf))) == NULL) {
			dprintf(LOG_ERR, "%s" "memory allocation failed "
				"for host %s", FNAME, host->name);
			goto bad;
		}
		memset(hconf, 0, sizeof(*hconf));
		TAILQ_INIT(&hconf->addr_list);
		TAILQ_INIT(&hconf->addr_binding_list);
		TAILQ_INIT(&hconf->prefix_list);
		TAILQ_INIT(&hconf->prefix_binding_list);
		hconf->next = host_conflist0;
		host_conflist0 = hconf;

		if ((hconf->name = strdup(host->name)) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to copy host name: %s",
				FNAME, host->name);
			goto bad;
		}

		for (cfl = host->params; cfl; cfl = cfl->next) {
			switch(cfl->type) {
			case DECL_DUID:
				if (hconf->duid.duid_id) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated DUID for %s",
						FNAME, configfilename,
						cfl->line, host->name);
					goto bad;
				}
				if ((configure_duid((char *)cfl->ptr,
						    &hconf->duid)) != 0) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"failed to configure "
						"DUID for %s", FNAME,
						configfilename, cfl->line,
						host->name);
					goto bad;
				}
				dprintf(LOG_DEBUG, "%s"
					"configure DUID for %s: %s", FNAME,
					host->name, duidstr(&hconf->duid));
				break;
			case DECL_PREFIX:
				if (add_address(&hconf->prefix_list, cfl->ptr)) {
					dprintf(LOG_ERR, "%s" "failed "
						"to configure prefix for %s",
						FNAME, host->name);
					goto bad;
				}
				break;
			case DECL_IAID:
				if (hconf->iaidinfo.iaid) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated IAID for %s",
						FNAME, configfilename,
						cfl->line, host->name);
					goto bad;
				} else
					hconf->iaidinfo.iaid = (u_int32_t)cfl->num;
				break;
			case DECL_RENEWTIME:
				if (hconf->iaidinfo.renewtime) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated renewtime for %s",
						FNAME, configfilename,
						cfl->line, host->name);
					goto bad;
				} else
					hconf->iaidinfo.renewtime = (u_int32_t)cfl->num;
				break;
			case DECL_REBINDTIME:
				if (hconf->iaidinfo.rebindtime) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated rebindtime for %s",
						FNAME, configfilename,
						cfl->line, host->name);
					goto bad;
				} else 
					hconf->iaidinfo.rebindtime = (u_int32_t)cfl->num;
				break;
			case DECL_ADDRESS:
				if (add_address(&hconf->addr_list, cfl->ptr)) {
					dprintf(LOG_ERR, "%s" "failed "
						"to configure ipv6address for %s",
						FNAME, host->name);
					goto bad;
				}
				break;
			case DECL_LINKLOCAL:
				if (IN6_IS_ADDR_UNSPECIFIED(&hconf->linklocal)) {
					dprintf(LOG_ERR, "%s" "%s:%d "
						"duplicated linklocal for %s",
						FNAME, configfilename,
						cfl->line, host->name);
					goto bad;
				} else 
					memcpy(&hconf->linklocal, cfl->ptr, 
							sizeof(hconf->linklocal) );
				break;
			default:
				dprintf(LOG_ERR, "%s: %d invalid host configuration for %s",
					configfilename, cfl->line, host->name);
				goto bad;
			}
		}
	}

	return (0);

  bad:
	/* there is currently nothing special to recover the error */
	return (-1);
}

int
configure_global_option(void)
{
	struct cf_list *cl;

	/* DNS servers */
	if (cf_dns_list && dhcp6_mode != DHCP6_MODE_SERVER) {
		dprintf(LOG_INFO, "%s" "%s:%d server-only configuration",
		    FNAME, configfilename, cf_dns_list->line);
		goto bad;
	}
	TAILQ_INIT(&dnslist0);
	for (cl = cf_dns_list; cl; cl = cl->next) {
		/* duplication check */
		if (dhcp6_find_listval(&dnslist0, cl->ptr,
		    DHCP6_LISTVAL_ADDR6)) {
			dprintf(LOG_INFO, "%s"
			    "%s:%d duplicated DNS server: %s", FNAME,
			    configfilename, cl->line,
			    in6addr2str((struct in6_addr *)cl->ptr, 0));
			goto bad;
		}
		if (dhcp6_add_listval(&dnslist0, cl->ptr,
		    DHCP6_LISTVAL_ADDR6) == NULL) {
			dprintf(LOG_ERR, "%s" "failed to add a DNS server", 
				FNAME);
			goto bad;
		}
	}

	return 0;

  bad:
	return -1;
}


void
configure_cleanup(void)
{
	clear_ifconf(dhcp6_ifconflist);
	dhcp6_ifconflist = NULL;
	clear_hostconf(host_conflist0);
	host_conflist0 = NULL;
	dhcp6_clear_list(&dnslist0);
	TAILQ_INIT(&dnslist0);
}

void
configure_commit(void)
{
	struct dhcp6_ifconf *ifc;
	struct dhcp6_if *ifp;

	/* commit interface configuration */
	for (ifc = dhcp6_ifconflist; ifc; ifc = ifc->next) {
		if ((ifp = find_ifconfbyname(ifc->ifname)) != NULL) {
			ifp->send_flags = ifc->send_flags;

			ifp->allow_flags = ifc->allow_flags;

			dhcp6_clear_list(&ifp->reqopt_list);
			ifp->reqopt_list = ifc->reqopt_list;
			TAILQ_INIT(&ifc->reqopt_list);
			
			dhcp6_clear_list(&ifp->addr_list);
			ifp->addr_list = ifc->addr_list;
			TAILQ_INIT(&ifc->addr_list);

			dhcp6_clear_list(&ifp->prefix_list);
			ifp->prefix_list = ifc->prefix_list;
			TAILQ_INIT(&ifc->prefix_list);
			
			clear_option_list(&ifp->option_list);
			ifp->option_list = ifc->option_list;
			TAILQ_INIT(&ifc->option_list);

			ifp->server_pref = ifc->server_pref;

			/*  added start pling 09/07/2010 */
			/* configure user-class */
			if (dhcp6_mode == DHCP6_MODE_CLIENT)
			    strcpy(ifp->user_class, ifc->user_class);
			/*  added end pling 09/07/2010 */

			memcpy(&ifp->iaidinfo, &ifc->iaidinfo, sizeof(ifp->iaidinfo));
		}
	}
	clear_ifconf(dhcp6_ifconflist);

	/* commit prefix configuration */
	if (host_conflist) {
		/* clear previous configuration. (need more work?) */
		clear_hostconf(host_conflist);
	}
	host_conflist = host_conflist0;
	host_conflist0 = NULL;
}

static void
clear_ifconf(struct dhcp6_ifconf *iflist)
{
	struct dhcp6_ifconf *ifc, *ifc_next;

	for (ifc = iflist; ifc; ifc = ifc_next) {
		ifc_next = ifc->next;

		free(ifc->ifname);

		dhcp6_clear_list(&ifc->reqopt_list);

		free(ifc);
	}
}

static void
clear_hostconf(struct host_conf *hlist)
{
	struct host_conf *host, *host_next;
	struct dhcp6_listval *p;

	for (host = hlist; host; host = host_next) {
		host_next = host->next;

		free(host->name);
		while ((p = TAILQ_FIRST(&host->prefix_list)) != NULL) {
			TAILQ_REMOVE(&host->prefix_list, p, link);
			free(p);
		}
		if (host->duid.duid_id)
			free(host->duid.duid_id);
		free(host);
	}
}

static int
add_options(int opcode,	struct dhcp6_ifconf *ifc,
	    struct cf_list *cfl0)
{
	struct dhcp6_listval *opt;
	struct cf_list *cfl;
	int opttype;

	for (cfl = cfl0; cfl; cfl = cfl->next) {
		if (opcode ==  DHCPOPTCODE_REQUEST) {
			for (opt = TAILQ_FIRST(&ifc->reqopt_list); opt;
			     opt = TAILQ_NEXT(opt, link)) {
				if (opt->val_num == cfl->type) {
					dprintf(LOG_INFO, "%s"
						"duplicated requested"
						" option: %s", FNAME,
						dhcp6optstr(cfl->type));
					goto next; /* ignore it */
				}
			}
		}

		switch(cfl->type) {
		case DHCPOPT_RAPID_COMMIT:
			switch(opcode) {
			case DHCPOPTCODE_SEND:
				ifc->send_flags |= DHCIFF_RAPID_COMMIT;
				break;
			case DHCPOPTCODE_ALLOW:
				ifc->allow_flags |= DHCIFF_RAPID_COMMIT;
				break;
			default:
				dprintf(LOG_ERR, "%s" "invalid operation (%d) "
					"for option type (%d)",
					FNAME, opcode, cfl->type);
				return (-1);
			}
			break;
		case DHCPOPT_PREFIX_DELEGATION:
			switch(opcode) {
			case DHCPOPTCODE_REQUEST:
				ifc->send_flags |= DHCIFF_PREFIX_DELEGATION;
				break;
			default:
				dprintf(LOG_ERR, "%s" "invalid operation (%d) "
					"for option type (%d)",
					FNAME, opcode, cfl->type);
				return (-1);
			}
			break;

		case DHCPOPT_DNS:
			switch(opcode) {
			case DHCPOPTCODE_REQUEST:
				opttype = DH6OPT_DNS_SERVERS;
				if (dhcp6_add_listval(&ifc->reqopt_list,
				    &opttype, DHCP6_LISTVAL_NUM) == NULL) {
					dprintf(LOG_ERR, "%s" "failed to "
					    "configure an option", FNAME);
					return (-1);
				}
				break;
			default:
				dprintf(LOG_ERR, "%s" "invalid operation (%d) "
					"for option type (%d)",
					FNAME, opcode, cfl->type);
				break;
			}
			break;

        /*  added start pling 09/23/2010 */
        /* To support domain search list for DHCPv6 readylogo tests */
        case DHCPOPT_DOMAIN_LIST:
            switch(opcode) {
                case DHCPOPTCODE_REQUEST:
                    opttype = DH6OPT_DOMAIN_LIST;
                    if (dhcp6_add_listval(&ifc->reqopt_list,
                        &opttype, DHCP6_LISTVAL_NUM) == NULL) {
                        dprintf(LOG_ERR, "%s" "failed to "
                            "configure an option", FNAME);
                        return (-1);
                    }
                    break;
                default:
                    dprintf(LOG_ERR, "%s" "invalid operation (%d) "
                        "for option type (%d)",
                        FNAME, opcode, cfl->type);
                    break;
            }
            break;
        /*  added end pling 09/23/2010 */

		default:
			dprintf(LOG_ERR, "%s"
				"unknown option type: %d", FNAME, cfl->type);
				return (-1);
		}

	next:;
	}

	return (0);
}

static int
add_address(struct dhcp6_list *addr_list,
	    struct dhcp6_addr *v6addr)
{
	struct dhcp6_listval *lv, *val;
	
	/* avoid invalid addresses */
	if (IN6_IS_ADDR_RESERVED(&v6addr->addr)) {
		dprintf(LOG_ERR, "%s" "invalid address: %s",
			FNAME, in6addr2str(&v6addr->addr, 0));
		return (-1);
	}

	/* address duplication check */
	for (lv = TAILQ_FIRST(addr_list); lv;
	     lv = TAILQ_NEXT(lv, link)) {
		if (IN6_ARE_ADDR_EQUAL(&lv->val_dhcp6addr.addr, &v6addr->addr) &&
		    lv->val_dhcp6addr.plen == v6addr->plen) {
			dprintf(LOG_ERR, "%s"
				"duplicated address: %s/%d ", FNAME,
				in6addr2str(&v6addr->addr, 0), v6addr->plen);
			return (-1);
		}
	}
	if ((val = (struct dhcp6_listval *)malloc(sizeof(*val))) == NULL)
		dprintf(LOG_ERR, "%s" "memory allocation failed", FNAME);
	memset(val, 0, sizeof(*val));
	memcpy(&val->val_dhcp6addr, v6addr, sizeof(val->val_dhcp6addr));
	dprintf(LOG_DEBUG, "%s" "add address: %s",
		FNAME, in6addr2str(&v6addr->addr, 0));
	TAILQ_INSERT_TAIL(addr_list, val, link);
	return (0);
}

int add_option (struct dhcp6_option_list *opts, struct cf_list *cfl)
{
	struct dhcp6_option *opt ;

	if ( get_if_option( opts, cfl->type ) != 0L )
		return (-1);

	switch (cfl->type)
	{
	case DECL_PREFIX_DELEGATION_INTERFACE:
		opt = (struct dhcp6_option*)malloc(sizeof(struct dhcp6_option));
		opt->type = cfl->type;
		if ( cfl->ptr != 0L )
		{
			opt->len = strlen((char*)(cfl->ptr))+1;
			opt->val = malloc(opt->len);
			memcpy(opt->val, cfl->ptr, opt->len);
		}else
		{
			opt->val = malloc(1);
			opt->len = 0;
			*((char*)(opt->val))='\0';
		}
		TAILQ_INSERT_TAIL(opts, opt, link);
		break;
	default:
	        break;
	}
	return (0);
}

int clear_option_list (struct dhcp6_option_list *opts)
{
	struct dhcp6_option *opt;
	while ((opt = TAILQ_FIRST(opts)) != NULL) {
		TAILQ_REMOVE(opts, opt, link);
		free(opt);
	}
}

void *get_if_option( struct dhcp6_option_list *opts, int type )
{
	struct dhcp6_option *opt;
	TAILQ_FOREACH( opt, opts, link)
	{
		if ( opt->type == type )
			return opt->val;
	}
	return 0L;
}
