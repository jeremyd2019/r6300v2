/*
 * Broadcom Home Gateway Reference Design
 * Broadcom Web Page Configuration Variables
 *
 * Copyright (C) 2011, Broadcom Corporation. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 * $Id: bcmcvar.h 241182 2011-02-17 21:50:03Z $
 */

#ifndef _bcmcvar_h
#define _bcmcvar_h

#define POST_BUF_SIZE	10000
#define WEBS_BUF_SIZE	5000
#define MAX_STA_COUNT	256
#define NVRAM_BUFSIZE	100

#define websBufferInit(wp) {webs_buf = malloc(WEBS_BUF_SIZE); webs_buf_offset = 0;}
#define websBufferWrite(wp, fmt, args...) {webs_buf_offset += \
	sprintf(webs_buf+webs_buf_offset, fmt , ## args);}
#define websBufferFlush(wp) {webs_buf[webs_buf_offset] = '\0'; fprintf(wp, webs_buf); fflush(wp); \
	free(webs_buf); webs_buf = NULL;}

#define ARGV(args...) ((char *[]) { args, NULL })
#define XSTR(s) STR(s)
#define STR(s) #s

struct variable {
	char *name;
	char *longname;
	char *prefix;
	void (*validate)(webs_t wp, char *value, struct variable *v, char *);
	char **argv;
	int nullok;
	int ezc_flags;
};

int variables_arraysize(void);

/* NVRAM macros and defines */

/* Uplad constraints for NVRAM file */
struct UPLOAD_CONSTRAINTS {
	char name[32];			/* name of constraint */
	char *(*get)(const char *);	/* get method for the constraint */
	char altval[32];	/* if primary match fail use this subject to modifiers below */
	int  flags;			/* match modifier flags */
};

typedef struct UPLOAD_CONSTRAINTS upload_constraints;

/* Upload constraint modifier flags */
#define	NVRAM_CONS_INT_GT	0x01	/* Convert to integer and match if filevar > imagevar */
#define NVRAM_CONS_INT_LT	0x02	/* Convert to integer and match if filevar < imagevar */
#define NVRAM_CONS_OPTIONAL 	0x04	/* Match if present in file */
#define NVRAM_CONS_ALT_MATCH	0x08	/* Do an exact match if altvar is specified ignored
					 * otherwise
					 */
#define NVRAM_CONS_AND_MORE	0x10	/* There are more AND match criteria to follow for the given
					 * var
					 */
#define NVRAM_CONS_OR_MORE	0x20	/* There are more OR match criteria to follow for the given
					 * var
					 */
#define NVRAM_CONS_PARTIAL_MATCH	0x40	/* Altval contains the partial patch string */

/* default initializer for constraint variables */
#define NULL_STR		{'\0'}

#define NVRAM_CONSTRAINT_VARS	{ 	{ "boardtype",	nvram_get,	NULL_STR,	0	}, \
		{ "boardrev",	nvram_get,	NULL_STR,	0	}, \
		{ "boardflags",	nvram_get,	NULL_STR,	0	}, \
		{ "os_name",	nvram_get,	NULL_STR,	0	}, \
		{ "os_version",	osversion_get,	"INTERNAL",	NVRAM_CONS_PARTIAL_MATCH	}, \
		{ NULL_STR,	NULL,		NULL_STR,	0				}\
		}
/*
 * NVRAM Validation control flags.
 * Also directs handling of the hodge podge of
 * NVRAM variable formats
 *
*/
#define NVRAM_ENCRYPT		0x10000000 /* Encrypt variable prior to saving NVRAM to file */
#define NVRAM_MP		0x20000000 /* NVRAM Variable is both single and multi instance eg
					    * wanXX_unit
					    */
#define NVRAM_WL_MULTI		0x40000000 /* Multi instance wireless variable wlXX_name */
#define NVRAM_WAN_MULTI		0x80000000 /* Multi instance wan variable wanXX_name */
#define NVRAM__IGNORE		0x01000000 /* Don't save or restore to NVRAM */
#define NVRAM_VLAN_MULTI	0x02000000 /* Multi instance VLAN variable vlanXXname */
#define NVRAM_GENERIC_MULTI	0x04000000 /* Port forward multi instance variables nameXX */
#define NVRAM_IGNORE	0x08000000 /* Skip NVRAM processing ie no save to file or validate */
#define NVRAM_MI		0x00100000 /* Multi instance NVRAM variable */
#define WEB_IGNORE		0x00200000 /* Ignore during web validation */
#define VIF_IGNORE		0x00400000 /* Ignore from wlx.y ->wl and from wl->wlx.y */

/* Markers for HTTP post buffer. Assumes HTTP multipart type encoding */
#define NVRAM_BEGIN_WEBFILE		"nvfile"
#define NVRAM_END_WEBFILE		'\0'

#define NVRAM_MAX_NETIF		8	/* Maximum number of suppoters NVRAM interfaces
					 * (wan,vlan,wl) of each type
					 */
#define NVRAM_MAX_STRINGSIZE	256

#define NVRAM_CHECKSUM_FILLER	"NVRAMTemporaryChecksumFiller"
#define NVRAM_CHECKSUM_LINENUM	1 	/* Line number where checksum is located starting from
					 * zero
					 */

#define NVRAM_PASSPHRASE	{0xdb, 0xca, 0xfe, 0xde, 0xbb, 0xde, 0xad, 0xbe, 0xef, 0x00}

#define NVRAM_ENCTAG		':'

#define NVRAM_SALTSIZE		8
#define NVRAM_HASHSIZE		20
#define NVRAM_FILECHKSUM_SIZE	NVRAM_SALTSIZE + NVRAM_HASHSIZE
#define NVRAM_SHA1BUFSIZE	80
#define NVRAM_FILEKEYSIZE	32


/* To expand the header,
*	1)Make entry in the NVRAM_FILEHEADER define
*  	2)Add line reference below if needed
*/
#define NVRAM_FILEHEADER	{	"NumVariables",	"Checksum",	\
					NULL_STR				\
				}

/* Line references to each of the header elements */
#define NVRAM_LINECOUNT_LINENUM	0
#define NVRAM_CHECKSUM_LINENUM	1

/*  Number of lines the header occupies. */
#define NVRAM_HEADER_LINECOUNT(hdr)	(sizeof((hdr)) / sizeof((hdr)[0]) - 1)

#endif /* _bcmcvar_h */
