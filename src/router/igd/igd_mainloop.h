/*
 * Broadcom IGD main loop include file
 *
 * Copyright (C) 2011, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: igd_mainloop.h 241182 2011-02-17 21:50:03Z $
 */

#ifndef __IGD_MAINLOOP_H__
#define __IGD_MAINLOOP_H__

#define	IGD_FLAG_SHUTDOWN	1
#define	IGD_FLAG_RESTART	2

int igd_mainloop(int port, int adv_time, IGD_NET *netlist);
void igd_stop_handler();
void igd_restart_handler();

#endif /* __IGD_MAINLOOP_H__ */
