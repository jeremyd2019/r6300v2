/*
 * Wireless network adapter utilities (RTE-specific)
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wl_rte_support.c 241182 2011-02-17 21:50:03Z $
 */

#include <typedefs.h>
#include <osl.h>
#include <wlioctl.h>
#include <wlutils.h>
#include <shutils.h>
#include <bcmnvram.h>

int
wl_ioctl(char *pDevName, int cmd, void *buf, int len)
{
	hndrte_dev_t *dev = NULL;

	if (pDevName == NULL || *pDevName == NULL)
		return NULL;

	if (!(dev = hndrte_get_dev(pDevName)))
		return NULL;

	return dev->dev_funcs->ioctl(dev, cmd, buf, len, NULL, NULL, 0);
}

int
wl_hwaddr(char *pDevName, unsigned char *hwaddr)
{
	return wl_ioctl(pDevName, RTEGHWADDR, hwaddr, ETHER_ADDR_LEN);
}
