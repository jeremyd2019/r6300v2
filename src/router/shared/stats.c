/*
 * Support for router statistics gathering
 *
 * Copyright (C) 2012, Broadcom Corporation. All Rights Reserved.
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
 *
 * $Id: stats.c 364585 2012-10-24 18:17:16Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <bcmnvram.h>
#include <shutils.h>

extern int http_post(const char *server, char *buf, size_t count);

#define BUFSPACE MAX_NVRAM_SPACE	/* # of bytes of buffer */

int
http_stats(const char *url)
{
	char *buf, *s;
	char **cur;
	char *secrets[] = { "os_server", "stats_server", "http_passwd", NULL };
	char *files[] = { "/proc/version", "/proc/meminfo", "/proc/cpuinfo", "/proc/interrupts",
			  "/proc/net/dev", "/proc/net/pppoe", "/proc/net/snmp", NULL };
	char *contents;

	if (!(buf = malloc(BUFSPACE)))
		return errno;

	/* Get NVRAM variables */
	nvram_getall(buf, BUFSPACE);
	for (s = buf; *s; s++) {
		for (cur = secrets; *cur; cur++) {
			if (!strncmp(s, *cur, strlen(*cur))) {
				s += strlen(*cur) + 1;
				while (*s)
					*s++ = '*';
				break;
			}
		}
		*(s += strlen(s)) = '&';
	}

	/* Dump interesting /proc entries */
	for (cur = files; *cur; cur++) {
		if ((contents = file2str(*cur))) {
			s += snprintf(s, buf + BUFSPACE - s, "%s=%s&", *cur, contents);
			free(contents);
		}
	}

	/* Report uptime */
	s += snprintf(s, buf + BUFSPACE - s, "uptime=%lu&", (unsigned long) time(NULL));

	/* Save */
	s += snprintf(s, buf + BUFSPACE - s, "action=save");
	buf[BUFSPACE-1] = '\0';

	/* Post to server */
	http_post(url ? : nvram_safe_get("stats_server"), buf, BUFSPACE);

	free(buf);
	return 0;
}
