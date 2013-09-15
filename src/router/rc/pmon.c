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
 * $Id: pmon.c 241182 2011-02-17 21:50:03Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <typedefs.h>
#include <dirent.h>
#include <assert.h>
#include <sys/types.h>
#include <ctype.h>

#include "pmon.h"

typedef struct pmon_info
{
	pid_t         pid;
	pmon_func_t   func;
} pmon_info_t;

static pmon_info_t ptable[PMON_MAX_ENTRIES];


void pmon_init(void)
{
	memset(ptable, 0, sizeof(ptable));
}

bool pmon_register(pid_t pid, pmon_func_t func)
{
	int i;

	assert(func != NULL);
	assert(pid != 0);
	
	for (i = 0; i < PMON_MAX_ENTRIES; i++) {
		if (ptable[i].func == NULL) {
			ptable[i].func = func;
			ptable[i].pid = pid;
			return TRUE;
		}
			
	}
		
	return FALSE;
}

bool pmon_unregister(pid_t pid)
{
	int i;

	assert(pid != 0);
	
	for (i = 0; i < PMON_MAX_ENTRIES; i++) {
		if (ptable[i].pid == pid) {
			ptable[i].func = NULL;
			ptable[i].pid = 0;
			break;
		}
	}
		
	return TRUE;
}

pmon_func_t pmon_get_func(pid_t pid)
{
	int i;

	assert(pid != 0);

	for (i = 0; i < PMON_MAX_ENTRIES; i++) {
		if (ptable[i].pid == pid) {
			return ptable[i].func;
			break;
		}
	}

	return NULL;
}

pid_t pmon_get_pid(pmon_func_t func)
{
	int i;

	assert(func != NULL);
	
	for (i = 0; i < PMON_MAX_ENTRIES; i++) {
		if (ptable[i].func == func) {
			return ptable[i].pid;
			break;
		}
	}

	return -1;
}
