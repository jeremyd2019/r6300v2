/*
 * Wireless network adapter utilities (ecos-specific)
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
 *
 * $Id: wl_ecos.c 241182 2011-02-17 21:50:03Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#include <typedefs.h>
#include <wlioctl.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>

typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#include <ethtool.h>


int
wl_ioctl(char *name, int cmd, void *buf, int len)
{
	struct ifreq ifr;
	wl_ioctl_t ioc;
	int ret = 0;
	int s;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		ret = -1;
	}

	/* do it */
	memset(&ioc, 0, sizeof(ioc));
	ioc.cmd = cmd;
	ioc.buf = buf;
	ioc.len = len;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	ifr.ifr_data = (caddr_t) &ioc;
	if ((ret = ioctl(s, SIOCDEVPRIVATE, &ifr)) < 0) {
		if (cmd != WLC_GET_MAGIC) {
			ret = -2;
		}
	}

	/* cleanup */
	close(s);

	return ret;
}

int
wl_get(void *wl, int cmd, void *buf, int len)
{
	return wl_ioctl(wl, cmd, buf, len);
}

int
wl_set(void *wl, int cmd, void *buf, int len)
{
	return wl_ioctl(wl, cmd, buf, len);
}

int
wl_get_dev_type(char *name, void *buf, int len)
{
	int s;
	int ret;
	struct ifreq ifr;
	struct ethtool_drvinfo info;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	/* get device type */
	memset(&info, 0, sizeof(info));
	info.cmd = ETHTOOL_GDRVINFO;
	ifr.ifr_data = (caddr_t)&info;
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	if ((ret = ioctl(s, SIOCETHTOOL, &ifr)) < 0) {
		*(char *)buf = '\0';
	} else
		strncpy(buf, info.driver, len);

	close(s);

	return ret;
}

int
wl_hwaddr(char *name, unsigned char *hwaddr)
{
	struct ifreq ifr;
	int ret = 0;
	int s;

	/* open socket to kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return -1;
	}

	/* do it */
	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	if ((ret = ioctl(s, SIOCGIFHWADDR, &ifr)) == 0)
		memcpy(hwaddr, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);

	/* cleanup */
	close(s);

	return ret;
}

/**
 * strnicmp - Case insensitive, length-limited string comparison
 * @s1: One string
 * @s2: The other string
 * @len: the maximum number of characters to compare
 */
int strnicmp(const char *s1, const char *s2, size_t len)
{
	/* Yes, Virginia, it had better be unsigned */
	unsigned char c1, c2;

	c1 = 0;	c2 = 0;
	if (len)
	{
		do
		{
			c1 = *s1; c2 = *s2;
			s1++; s2++;
			if (!c1)
				break;
			if (!c2)
				break;
			if (c1 == c2)
				continue;
			c1 = tolower(c1);
			c2 = tolower(c2);
			if (c1 != c2)
				break;
		} while (--len);
	}
	return (int)c1 - (int)c2;
}

int stricmp(const char *s1, const char *s2)
{
	return (strnicmp(s1, s2, strlen(s1)));
}
