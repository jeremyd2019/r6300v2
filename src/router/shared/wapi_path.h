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
 * $Id: $
 */

#ifndef _WAPI_PATH_H_
#define _WAPI_PATH_H_


#include <confmtd_utils.h>


/* WAPI PATH */
#define WAPI_CONFIG_DIR			"config"
#define WAPI_DIR			RAMFS_CONFMTD_DIR"/wapi"
#define WAPI_WAI_DIR			WAPI_DIR"/"WAPI_CONFIG_DIR
#define WAPI_AS_DIR			WAPI_WAI_DIR"/as1000"

#define WAPI_AS_CER_FILE		WAPI_AS_DIR"/as.cer"
#define WAPI_AS_CRL_FILE		WAPI_AS_DIR"/as.crl"
#define WAPI_AS_CERLIB_FILE		WAPI_AS_DIR"/cerlib.iwn"
#define WAPI_AS_TIMECLICK_FILE		WAPI_AS_DIR"/timeclick.iwn"
#define WAPI_AS_THEASCER_FILE		WAPI_AS_DIR"/theascer.iwn"

#endif /* _WAPI_PATH_H_ */
