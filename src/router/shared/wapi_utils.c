/*
 * WAPI mtd read/write utility functions
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
 * $Id: wapi_utils.c 241182 2011-02-17 21:50:03Z $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <sys/ioctl.h>

#ifdef LINUX26
#include <mtd/mtd-user.h>
#else /* LINUX26 */
#include <linux/mtd/mtd.h>
#endif /* LINUX26 */

#include <bcmconfig.h>
#include <wapi_utils.h>

static unsigned short
wapi_checksum(const char *data, int datalen)
{
	unsigned short checksum = 0;
	unsigned short *ptr = (unsigned short *)data;
	int len = datalen;

	while (len > 0) {
		if (len == 1)
			checksum += (*ptr & 0xff00);
		else
			checksum += *ptr;
		ptr++;
		len -= 2;
	}
	return checksum;
}

/*
 * Open an MTD device
 * @param	mtd	path to or partition name of MTD device
 * @param	flags	open() flags
 * @return	return value of open()
 */
int
wapi_mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[PATH_MAX];
	int i;

	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, mtd)) {
#ifdef LINUX26
				snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
#else
				snprintf(dev, sizeof(dev), "/dev/mtd/%d", i);
#endif
				fclose(fp);
				return open(dev, flags);
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

/*
 * Write a file to an MTD device
 * @param	path	file to write or a URL
 * @param	mtd	path to or partition name of MTD device
 * @return	0 on success and errno on failure
 */
int
wapi_mtd_backup()
{
	char *cmd = "tar cf - "CONFIG_DIR" -C "RAMFS_WAPI_DIR" | gzip -c > "WAPI_TGZ_TMP_FILE;
	mtd_info_t mtd_info;
	erase_info_t erase_info;
	int mtd_fd = -1;
	FILE *fp = NULL;
	char *buf = NULL;
	struct stat tmp_stat;
	int ret = -1;
	wapi_mtd_hdr_t mtd_hdr;

	/* backup wapi directiries to raw partition */
	unlink(WAPI_TGZ_TMP_FILE);

	/* Open MTD device and get sector size */
	if ((mtd_fd = wapi_mtd_open("wapi", O_RDWR)) < 0 ||
	    ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0) {
		perror("wapi");
		goto fail;
	}

	/* create as tar file */
	errno = 0;
	system(cmd);
	if (stat(WAPI_TGZ_TMP_FILE, &tmp_stat) ||
	    errno == ENOENT) {
		perror("tgz");
		goto fail;
	}

	if ((tmp_stat.st_size + sizeof(wapi_mtd_hdr_t)) > mtd_info.size || tmp_stat.st_size == 0) {
		perror("size");
		goto fail;
	}

	/* Allocate temporary buffer */
	if ((buf = malloc(tmp_stat.st_size)) == NULL) {
		perror("malloc");
		goto fail;
	}

	fp = fopen(WAPI_TGZ_TMP_FILE, "r");
	if (fp == NULL) {
		fprintf(stderr, "%s: can't open file\n", WAPI_TGZ_TMP_FILE);
		goto fail;
	}

	if (fread(buf, 1, tmp_stat.st_size, fp) != tmp_stat.st_size) {
		perror("read");
		goto fail;
	}

	erase_info.start = 0;
	erase_info.length = mtd_info.size;

	/* create mtd header content */
	memcpy(&mtd_hdr.magic, WAPI_MTD_MAGIC, 4);
	mtd_hdr.len = tmp_stat.st_size;
	mtd_hdr.checksum = wapi_checksum(buf, tmp_stat.st_size);

	/* Do it */
	(void) ioctl(mtd_fd, MEMUNLOCK, &erase_info);
	if (ioctl(mtd_fd, MEMERASE, &erase_info) != 0 ||
	    write(mtd_fd, &mtd_hdr, sizeof(wapi_mtd_hdr_t)) != sizeof(wapi_mtd_hdr_t) ||
	    write(mtd_fd, buf, tmp_stat.st_size) != tmp_stat.st_size) {
		perror("write");
		goto fail;
	}

	printf("update wapi partition OK\n");
	ret = 0;

fail:
	unlink(WAPI_TGZ_TMP_FILE);

	if (buf)
		free(buf);

	if (mtd_fd >= 0)
		close(mtd_fd);

	if (fp)
		fclose(fp);

	return ret;
}

/*
 * Write a file to an MTD device
 * @param	path	file to write or a URL
 * @param	mtd	path to or partition name of MTD device
 * @return	0 on success and errno on failure
 */
int
wapi_mtd_restore()
{
	char *cmd = "gunzip -c "WAPI_TGZ_TMP_FILE" | tar xf - -C "RAMFS_WAPI_DIR;
	mtd_info_t mtd_info;
	int mtd_fd = -1;
	FILE *fp = NULL;
	char *buf = NULL;
	int ret = -1;
	wapi_mtd_hdr_t mtd_hdr = {0};

	/* create wapi directory */
	mkdir(RAMFS_WAPI_DIR, 0777);

	/* Open MTD device and get sector size */
	if ((mtd_fd = wapi_mtd_open("wapi", O_RDWR)) < 0 ||
	    ioctl(mtd_fd, MEMGETINFO, &mtd_info) != 0) {
		printf("mtd");
		goto fail;
	}

	read(mtd_fd, &mtd_hdr, sizeof(wapi_mtd_hdr_t));

	if (memcmp(&mtd_hdr.magic, WAPI_MTD_MAGIC, 4)) {
		printf("magic incorrect");
		goto fail;
	}

	if (mtd_hdr.len > mtd_info.size) {
		printf("size too long");
		goto fail;
	}

	/* Allocate temporary buffer */
	if ((buf = malloc(mtd_hdr.len)) == NULL) {
		printf("buffer");
		goto fail;
	}
	read(mtd_fd, buf, mtd_hdr.len);

	if (wapi_checksum(buf, mtd_hdr.len) != mtd_hdr.checksum) {
		printf("checksum");
		goto fail;
	}

	/* write mtd data to tar file */
	if ((fp = fopen(WAPI_TGZ_TMP_FILE, "w")) == NULL) {
		printf("%s: can't open file\n", WAPI_TGZ_TMP_FILE);
		goto fail;
	}
	fwrite(buf, 1, mtd_hdr.len, fp);
	fclose(fp);

	/* untar wapi directory */
	system(cmd);

	unlink(WAPI_TGZ_TMP_FILE);

	ret = 0;
fail:
	if (buf)
		free(buf);

	if (mtd_fd >= 0)
		close(mtd_fd);

	if (ret) {
		/* create directory */
		mkdir(WAPI_WAI_DIR, 0777);
		mkdir(WAPI_AS_DIR, 0777);
	}

	return ret;
}
