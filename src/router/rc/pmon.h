/*
 * Process monitor
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
 * $Id: pmon.h 241182 2011-02-17 21:50:03Z $
 */

#ifndef _pmon_h_
#define _pmon_h_

#define PMON_MAX_ENTRIES     5

typedef int (*pmon_func_t)(void);


void pmon_init(void);

bool pmon_register(pid_t pid, pmon_func_t func);

bool pmon_unregister(pid_t pid);

pmon_func_t pmon_get_func(pid_t pid);

pid_t pmon_get_pid(pmon_func_t func);
	
#endif /* _pmon_h_ */
