/* Library which manipulates firewall rules.  Version 0.1. */

/* Architecture of firewall rules is as follows:
 *
 * Chains go INPUT, FORWARD, OUTPUT then user chains.
 * Each user chain starts with an ERROR node.
 * Every chain ends with an unconditional jump: a RETURN for user chains,
 * and a POLICY for built-ins.
 */

/* (C)1999 Paul ``Rusty'' Russell - Placed under the GNU GPL (See
   COPYING for details). */

#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG_CONNTRACK
#define inline
#endif

#if !defined(__GLIBC__) || (__GLIBC__ < 2)
typedef unsigned int socklen_t;
#endif

#include "libiptc/libiptc.h"

#define IP_VERSION	4
#define IP_OFFSET	0x1FFF

#define HOOK_PRE_ROUTING	NF_IP_PRE_ROUTING
#define HOOK_LOCAL_IN		NF_IP_LOCAL_IN
#define HOOK_FORWARD		NF_IP_FORWARD
#define HOOK_LOCAL_OUT		NF_IP_LOCAL_OUT
#define HOOK_POST_ROUTING	NF_IP_POST_ROUTING
#ifdef NF_IP_DROPPING
#define HOOK_DROPPING		NF_IP_DROPPING
#endif

#define STRUCT_ENTRY_TARGET	struct ipt_entry_target
#define STRUCT_ENTRY		struct ipt_entry
#define STRUCT_ENTRY_MATCH	struct ipt_entry_match
#define STRUCT_GETINFO		struct ipt_getinfo
#define STRUCT_GET_ENTRIES	struct ipt_get_entries
#define STRUCT_COUNTERS		struct ipt_counters
#define STRUCT_COUNTERS_INFO	struct ipt_counters_info
#define STRUCT_STANDARD_TARGET	struct ipt_standard_target
#define STRUCT_REPLACE		struct ipt_replace

#define STRUCT_TC_HANDLE	struct iptc_handle
#define xtc_handle		iptc_handle

#define ENTRY_ITERATE		IPT_ENTRY_ITERATE
#define TABLE_MAXNAMELEN	IPT_TABLE_MAXNAMELEN
#define FUNCTION_MAXNAMELEN	IPT_FUNCTION_MAXNAMELEN

#define GET_TARGET		ipt_get_target

#define ERROR_TARGET		IPT_ERROR_TARGET
#define NUMHOOKS		NF_IP_NUMHOOKS

#define IPT_CHAINLABEL		ipt_chainlabel

#define TC_DUMP_ENTRIES		dump_entries
#define TC_IS_CHAIN		iptc_is_chain
#define TC_FIRST_CHAIN		iptc_first_chain
#define TC_NEXT_CHAIN		iptc_next_chain
#define TC_FIRST_RULE		iptc_first_rule
#define TC_NEXT_RULE		iptc_next_rule
#define TC_GET_TARGET		iptc_get_target
#define TC_BUILTIN		iptc_builtin
#define TC_GET_POLICY		iptc_get_policy
#define TC_INSERT_ENTRY		iptc_insert_entry
#define TC_REPLACE_ENTRY	iptc_replace_entry
#define TC_APPEND_ENTRY		iptc_append_entry
#define TC_CHECK_ENTRY		iptc_check_entry
#define TC_DELETE_ENTRY		iptc_delete_entry
#define TC_DELETE_NUM_ENTRY	iptc_delete_num_entry
#define TC_FLUSH_ENTRIES	iptc_flush_entries
#define TC_ZERO_ENTRIES		iptc_zero_entries
#define TC_READ_COUNTER		iptc_read_counter
#define TC_ZERO_COUNTER		iptc_zero_counter
#define TC_SET_COUNTER		iptc_set_counter
#define TC_CREATE_CHAIN		iptc_create_chain
#define TC_GET_REFERENCES	iptc_get_references
#define TC_DELETE_CHAIN		iptc_delete_chain
#define TC_RENAME_CHAIN		iptc_rename_chain
#define TC_SET_POLICY		iptc_set_policy
#define TC_GET_RAW_SOCKET	iptc_get_raw_socket
#define TC_INIT			iptc_init
#define TC_FREE			iptc_free
#define TC_COMMIT		iptc_commit
#define TC_STRERROR		iptc_strerror
#define TC_NUM_RULES		iptc_num_rules
#define TC_GET_RULE		iptc_get_rule

#define TC_AF			AF_INET
#define TC_IPPROTO		IPPROTO_IP

#define SO_SET_REPLACE		IPT_SO_SET_REPLACE
#define SO_SET_ADD_COUNTERS	IPT_SO_SET_ADD_COUNTERS
#define SO_GET_INFO		IPT_SO_GET_INFO
#define SO_GET_ENTRIES		IPT_SO_GET_ENTRIES
#define SO_GET_VERSION		IPT_SO_GET_VERSION

#define STANDARD_TARGET		IPT_STANDARD_TARGET
#define LABEL_RETURN		IPTC_LABEL_RETURN
#define LABEL_ACCEPT		IPTC_LABEL_ACCEPT
#define LABEL_DROP		IPTC_LABEL_DROP
#define LABEL_QUEUE		IPTC_LABEL_QUEUE

#define ALIGN			XT_ALIGN
#define RETURN			IPT_RETURN

#include "libiptc.c"

#define IP_PARTS_NATIVE(n)			\
(unsigned int)((n)>>24)&0xFF,			\
(unsigned int)((n)>>16)&0xFF,			\
(unsigned int)((n)>>8)&0xFF,			\
(unsigned int)((n)&0xFF)

#define IP_PARTS(n) IP_PARTS_NATIVE(ntohl(n))

static int
dump_entry(struct ipt_entry *e, struct iptc_handle *const handle)
{
	size_t i;
	STRUCT_ENTRY_TARGET *t;

	printf("Entry %u (%lu):\n", iptcb_entry2index(handle, e),
	       iptcb_entry2offset(handle, e));
	printf("SRC IP: %u.%u.%u.%u/%u.%u.%u.%u\n",
	       IP_PARTS(e->ip.src.s_addr),IP_PARTS(e->ip.smsk.s_addr));
	printf("DST IP: %u.%u.%u.%u/%u.%u.%u.%u\n",
	       IP_PARTS(e->ip.dst.s_addr),IP_PARTS(e->ip.dmsk.s_addr));
	printf("Interface: `%s'/", e->ip.iniface);
	for (i = 0; i < IFNAMSIZ; i++)
		printf("%c", e->ip.iniface_mask[i] ? 'X' : '.');
	printf("to `%s'/", e->ip.outiface);
	for (i = 0; i < IFNAMSIZ; i++)
		printf("%c", e->ip.outiface_mask[i] ? 'X' : '.');
	printf("\nProtocol: %u\n", e->ip.proto);
	printf("Flags: %02X\n", e->ip.flags);
	printf("Invflags: %02X\n", e->ip.invflags);
	printf("Counters: %llu packets, %llu bytes\n",
	       (unsigned long long)e->counters.pcnt, (unsigned long long)e->counters.bcnt);
	printf("Cache: %08X\n", e->nfcache);

	IPT_MATCH_ITERATE(e, print_match);

	t = GET_TARGET(e);
	printf("Target name: `%s' [%u]\n", t->u.user.name, t->u.target_size);
	if (strcmp(t->u.user.name, STANDARD_TARGET) == 0) {
		const unsigned char *data = t->data;
		int pos = *(const int *)data;
		if (pos < 0)
			printf("verdict=%s\n",
			       pos == -NF_ACCEPT-1 ? "NF_ACCEPT"
			       : pos == -NF_DROP-1 ? "NF_DROP"
			       : pos == -NF_QUEUE-1 ? "NF_QUEUE"
			       : pos == RETURN ? "RETURN"
			       : "UNKNOWN");
		else
			printf("verdict=%u\n", pos);
	} else if (strcmp(t->u.user.name, IPT_ERROR_TARGET) == 0)
		printf("error=`%s'\n", t->data);

	printf("\n");
	return 0;
}

static unsigned char *
is_same(const STRUCT_ENTRY *a, const STRUCT_ENTRY *b, unsigned char *matchmask)
{
	unsigned int i;
	unsigned char *mptr;

	/* Always compare head structures: ignore mask here. */
	if (a->ip.src.s_addr != b->ip.src.s_addr
	    || a->ip.dst.s_addr != b->ip.dst.s_addr
	    || a->ip.smsk.s_addr != b->ip.smsk.s_addr
	    || a->ip.dmsk.s_addr != b->ip.dmsk.s_addr
	    || a->ip.proto != b->ip.proto
	    || a->ip.flags != b->ip.flags
	    || a->ip.invflags != b->ip.invflags)
		return NULL;

	for (i = 0; i < IFNAMSIZ; i++) {
		if (a->ip.iniface_mask[i] != b->ip.iniface_mask[i])
			return NULL;
		if ((a->ip.iniface[i] & a->ip.iniface_mask[i])
		    != (b->ip.iniface[i] & b->ip.iniface_mask[i]))
			return NULL;
		if (a->ip.outiface_mask[i] != b->ip.outiface_mask[i])
			return NULL;
		if ((a->ip.outiface[i] & a->ip.outiface_mask[i])
		    != (b->ip.outiface[i] & b->ip.outiface_mask[i]))
			return NULL;
	}

	if (a->target_offset != b->target_offset
	    || a->next_offset != b->next_offset)
		return NULL;

	mptr = matchmask + sizeof(STRUCT_ENTRY);
	if (IPT_MATCH_ITERATE(a, match_different, a->elems, b->elems, &mptr))
		return NULL;
	mptr += XT_ALIGN(sizeof(struct ipt_entry_target));

	return mptr;
}
