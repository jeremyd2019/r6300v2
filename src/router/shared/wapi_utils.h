/*
 * Shell-like utility functions
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
 * $Id: wapi_utils.h 241182 2011-02-17 21:50:03Z $
 */

#ifndef _WAPI_UTILS_H_
#define _WAPI_UTILS_H_

/* WAPI ramfs directories */
#define RAMFS_WAPI_DIR			__CONFIG_WAPI_CONF__
#define CONFIG_DIR			"config"
#define WAPI_WAI_DIR			RAMFS_WAPI_DIR"/"CONFIG_DIR
#define WAPI_AS_DIR			RAMFS_WAPI_DIR"/"CONFIG_DIR"/as1000"

#define WAPI_TGZ_TMP_FILE		RAMFS_WAPI_DIR"/config.tgz"

#define WAPI_AS_CER_FILE		WAPI_AS_DIR"/as.cer"

/* WAPI partition magic number: "wapi" */
#define WAPI_MTD_MAGIC			"\077\061\070\069"

typedef struct {
	unsigned int magic;
	unsigned int len;
	unsigned short checksum;
} wapi_mtd_hdr_t;

/*
 * Write a file to an MTD device
 * @param	path	file to write or a URL
 * @param	mtd	path to or partition name of MTD device
 * @return	0 on success and errno on failure
 */
int wapi_mtd_backup();

/*
 * Read MTD device to file
 * @param	path	file to write or a URL
 * @param	mtd	path to or partition name of MTD device
 * @return	0 on success and errno on failure
 */
int wapi_mtd_restore();

#endif /* _WAPI_UTILS_H_ */
