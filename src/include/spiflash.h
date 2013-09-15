/*
 * Broadcom QSPI serial flash interface
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

#ifndef _spiflash_h_
#define _spiflash_h_

#include <typedefs.h>
#include <sbchipc.h>
#include <qspi_core.h>

struct spiflash {
	uint blocksize;		/* Block size */
	uint numblocks;		/* Number of blocks */
	uint32 type;		/* Type */
	uint size;		/* Total size in bytes */
	uint32 base;
};

#ifndef FLASH_SPI_MAX_PAGE_SIZE
#define FLASH_SPI_MAX_PAGE_SIZE   (256)
#endif

/* Utility functions */
extern int spiflash_open(si_t *sih, qspiregs_t *qspi);
extern int spiflash_read(si_t *sih, qspiregs_t *qspi,
                       uint offset, uint len, uchar *buf);
extern int spiflash_write(si_t *sih, qspiregs_t *qspi,
                        uint offset, uint len, const uchar *buf);
extern int spiflash_erase(si_t *sih, qspiregs_t *qspi, uint offset);
extern int spiflash_commit(si_t *sih, qspiregs_t *qspi,
                         uint offset, uint len, const uchar *buf);
extern struct spiflash *spiflash_init(si_t *sih, qspiregs_t *qspi);

#endif /* _spiflash_h_ */
