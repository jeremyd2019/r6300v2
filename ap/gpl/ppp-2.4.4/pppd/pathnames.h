/*
 * define path names
 *
 * $Id: pathnames.h,v 1.18 2005/08/25 23:59:34 paulus Exp $
 */

#ifdef HAVE_PATHS_H
#include <paths.h>

#else /* HAVE_PATHS_H */
#ifndef _PATH_VARRUN
#define _PATH_VARRUN 	"/tmp/ppp/"
#endif
#define _PATH_DEVNULL	"/dev/null"
#endif /* HAVE_PATHS_H */

#ifndef _ROOT_PATH
#define _ROOT_PATH
#endif

#define _PATH_UPAPFILE 	 _ROOT_PATH "/tmp/ppp/pap-secrets"
#define _PATH_CHAPFILE 	 _ROOT_PATH "/tmp/ppp/chap-secrets"
#define _PATH_SRPFILE 	 _ROOT_PATH "/tmp/ppp/srp-secrets"

/*  added start, zacker, 04/20/2011 */
#define _PATH_UPAPFILE_IPV6 	 _ROOT_PATH "/tmp/ppp/pap-secrets-ipv6"
#define _PATH_CHAPFILE_IPV6 	 _ROOT_PATH "/tmp/ppp/chap-secrets-ipv6"
#define _PATH_SRPFILE_IPV6 	 _ROOT_PATH "/tmp/ppp/srp-secrets-ipv6"

extern char *path_upapfile;
extern char *path_chapfile;
extern char *path_srpfile;
/*  added end, zacker, 04/20/2011 */

#ifdef MULTIPLE_PPPOE
#define _PATH_SYSOPTIONS _ROOT_PATH "/tmp/ppp/opt" /*  wklin modified,
                                                      09/06/2007 */
#else
#define _PATH_SYSOPTIONS _ROOT_PATH "/tmp/ppp/options"
#endif
#define _PATH_IPUP	 _ROOT_PATH "/tmp/ppp/ip-up"
#define _PATH_IPDOWN	 _ROOT_PATH "/tmp/ppp/ip-down"
#define _PATH_IPPREUP	 _ROOT_PATH "/tmp/ppp/ip-pre-up"
#define _PATH_AUTHUP	 _ROOT_PATH "/tmp/ppp/auth-up"
#define _PATH_AUTHDOWN	 _ROOT_PATH "/tmp/ppp/auth-down"
#define _PATH_TTYOPT	 _ROOT_PATH "/tmp/ppp/options."
/* #define _PATH_CONNERRS	 _ROOT_PATH "/tmp/ppp/connect-errors" */
#define _PATH_CONNERRS	 _ROOT_PATH "/dev/null" /*wklin modified, 06/08/2007*/
#define _PATH_PEERFILES	 _ROOT_PATH "/tmp/ppp/peers/"
#define _PATH_RESOLV	 _ROOT_PATH "/tmp/ppp/resolv.conf"

#define _PATH_USEROPT	 ".ppprc"
#define	_PATH_PSEUDONYM	 ".ppp_pseudonym"

#ifdef INET6
#define _PATH_IPV6UP     _ROOT_PATH "/tmp/ppp/ipv6-up"
#define _PATH_IPV6DOWN   _ROOT_PATH "/tmp/ppp/ipv6-down"
#endif

#ifdef IPX_CHANGE
#define _PATH_IPXUP	 _ROOT_PATH "/tmp/ppp/ipx-up"
#define _PATH_IPXDOWN	 _ROOT_PATH "/tmp/ppp/ipx-down"
#endif /* IPX_CHANGE */

#ifdef __STDC__
#define _PATH_PPPDB	_ROOT_PATH _PATH_VARRUN "pppd2.tdb"
#else /* __STDC__ */
#ifdef HAVE_PATHS_H
#define _PATH_PPPDB	"/var/run/pppd2.tdb"
#else
#define _PATH_PPPDB	"/tmp/ppp/pppd2.tdb"
#endif
#endif /* __STDC__ */

#ifdef PLUGIN
#ifdef __STDC__
#define _PATH_PLUGIN	DESTDIR "/lib/pppd/" VERSION
#else /* __STDC__ */
#define _PATH_PLUGIN	"/usr/lib/pppd"
#endif /* __STDC__ */

#endif /* PLUGIN */
