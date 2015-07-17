/*
 *  linux/fs/hfsplus/wrapper.c
 *
 * Copyright (C) 2001
 * Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 * Handling of HFS wrappers around HFS+ volumes
 */

#include <linux/fs.h>
#include <linux/blkdev.h>
#include <linux/cdrom.h>
#include <linux/genhd.h>
#include <asm/unaligned.h>

#include "hfsplus_fs.h"
#include "hfsplus_raw.h"

struct hfsplus_wd {
	u32 ablk_size;
	u16 ablk_start;
	u16 embed_start;
	u16 embed_count;
};

static void hfsplus_end_io_sync(struct bio *bio, int err)
{
	if (err)
		clear_bit(BIO_UPTODATE, &bio->bi_flags);
	complete(bio->bi_private);
}

int hfsplus_submit_bio(struct block_device *bdev, sector_t sector,
		void *data, int rw)
{
	DECLARE_COMPLETION_ONSTACK(wait);
	struct bio *bio;
	int ret = 0;

	bio = bio_alloc(GFP_NOIO, 1);
	bio->bi_sector = sector;
	bio->bi_bdev = bdev;
	bio->bi_end_io = hfsplus_end_io_sync;
	bio->bi_private = &wait;

	/*
	 * We always submit one sector at a time, so bio_add_page must not fail.
	 */
	if (bio_add_page(bio, virt_to_page(data), HFSPLUS_SECTOR_SIZE,
			 offset_in_page(data)) != HFSPLUS_SECTOR_SIZE)
		BUG();

	submit_bio(rw, bio);
	wait_for_completion(&wait);

	if (!bio_flagged(bio, BIO_UPTODATE))
		ret = -EIO;

	bio_put(bio);
	return ret;
}

static int hfsplus_read_mdb(void *bufptr, struct hfsplus_wd *wd)
{
	u32 extent;
	u16 attrib;
	__be16 sig;

	sig = *(__be16 *)(bufptr + HFSP_WRAPOFF_EMBEDSIG);
	if (sig != cpu_to_be16(HFSPLUS_VOLHEAD_SIG) &&
	    sig != cpu_to_be16(HFSPLUS_VOLHEAD_SIGX))
		return 0;

	attrib = be16_to_cpu(*(__be16 *)(bufptr + HFSP_WRAPOFF_ATTRIB));
	if (!(attrib & HFSP_WRAP_ATTRIB_SLOCK) ||
	   !(attrib & HFSP_WRAP_ATTRIB_SPARED))
		return 0;

	wd->ablk_size = be32_to_cpu(*(__be32 *)(bufptr + HFSP_WRAPOFF_ABLKSIZE));
	if (wd->ablk_size < HFSPLUS_SECTOR_SIZE)
		return 0;
	if (wd->ablk_size % HFSPLUS_SECTOR_SIZE)
		return 0;
	wd->ablk_start = be16_to_cpu(*(__be16 *)(bufptr + HFSP_WRAPOFF_ABLKSTART));

	extent = get_unaligned_be32(bufptr + HFSP_WRAPOFF_EMBEDEXT);
	wd->embed_start = (extent >> 16) & 0xFFFF;
	wd->embed_count = extent & 0xFFFF;

	return 1;
}

static int hfsplus_get_last_session(struct super_block *sb,
				    sector_t *start, sector_t *size)
{
	struct cdrom_multisession ms_info;
	struct cdrom_tocentry te;
	int res;

	/* default values */
	*start = 0;
	*size = sb->s_bdev->bd_inode->i_size >> 9;

	if (HFSPLUS_SB(sb).session >= 0) {
		te.cdte_track = HFSPLUS_SB(sb).session;
		te.cdte_format = CDROM_LBA;
		res = ioctl_by_bdev(sb->s_bdev, CDROMREADTOCENTRY, (unsigned long)&te);
		if (!res && (te.cdte_ctrl & CDROM_DATA_TRACK) == 4) {
			*start = (sector_t)te.cdte_addr.lba << 2;
			return 0;
		}
		printk(KERN_ERR "hfs: invalid session number or type of track\n");
		return -EINVAL;
	}
	ms_info.addr_format = CDROM_LBA;
	res = ioctl_by_bdev(sb->s_bdev, CDROMMULTISESSION, (unsigned long)&ms_info);
	if (!res && ms_info.xa_flag)
		*start = (sector_t)ms_info.addr.lba << 2;
	return 0;
}

/* Find the volume header and fill in some minimum bits in superblock */
/* Takes in super block, returns true if good data read */
int hfsplus_read_wrapper(struct super_block *sb)
{
	struct buffer_head *bh;
	struct hfsplus_vh *vhdr;
	struct hfsplus_wd wd;
	sector_t part_start, part_size;
	u32 blocksize;
	int error = 0;

	error = -EINVAL;
	blocksize = sb_min_blocksize(sb, HFSPLUS_SECTOR_SIZE);
	if (!blocksize)
		goto out;

	if (hfsplus_get_last_session(sb, &part_start, &part_size))
		goto out;
printk(KERN_EMERG"part_start=%x, part_size=%x",part_start,part_size);
#if 0
	if ((u64)part_start + part_size > 0x100000000ULL) {
		pr_err("hfs: volumes larger than 2TB are not supported yet\n");
		goto out;
	}
#endif
  HFSPLUS_SB(sb).s_vhdr=NULL;
	
	error = -ENOMEM;
/*
	HFSPLUS_SB(sb).s_vhdr = kmalloc(HFSPLUS_SECTOR_SIZE, GFP_KERNEL);
	if (!HFSPLUS_SB(sb).s_vhdr)
		goto out;
	HFSPLUS_SB(sb).s_backup_vhdr = kmalloc(HFSPLUS_SECTOR_SIZE, GFP_KERNEL);
	if (!HFSPLUS_SB(sb).s_backup_vhdr)
		goto out_free_vhdr;
*/
/* Removed by Foxconn Antony 08/20/2013 end */

reread:
		bh = sb_bread512(sb, part_start + HFSPLUS_VOLHEAD_SECTOR, vhdr);
		if (!bh)
		{
  		    goto out_free_backup_vhdr;
    	}
    
	error = -EINVAL;
	switch (vhdr->signature) 
	{
	case cpu_to_be16(HFSPLUS_VOLHEAD_SIGX):
		set_bit(HFSPLUS_SB_HFSX, &HFSPLUS_SB(sb).flags);
		/*FALLTHRU*/
	case cpu_to_be16(HFSPLUS_VOLHEAD_SIG):
		break;
	case cpu_to_be16(HFSP_WRAP_MAGIC):
		if (!hfsplus_read_mdb(vhdr, &wd))
			goto out_free_backup_vhdr;
		wd.ablk_size >>= HFSPLUS_SECTOR_SHIFT;
		part_start += wd.ablk_start + wd.embed_start * wd.ablk_size;
		part_size = wd.embed_count * wd.ablk_size;
		brelse(bh);
		goto reread;
	default:
		/*
		 * Check for a partition block.
		 *
		 * (should do this only for cdrom/loop though)
		 */
		brelse(bh);
		if (hfs_part_find(sb, &part_start, &part_size))
		{
			goto out_free_backup_vhdr;
		}
		goto reread;
	}


	blocksize = be32_to_cpu(vhdr->blocksize);
	brelse(bh);

	/* block size must be at least as large as a sector
	 * and a multiple of 2
	 */
	if (blocksize < HFSPLUS_SECTOR_SIZE ||
	    ((blocksize - 1) & blocksize))
		goto out_free_backup_vhdr;
	HFSPLUS_SB(sb).alloc_blksz = blocksize;
	HFSPLUS_SB(sb).alloc_blksz_shift = 0;
	while ((blocksize >>= 1) != 0)
		HFSPLUS_SB(sb).alloc_blksz_shift++;
	blocksize = min(HFSPLUS_SB(sb).alloc_blksz, (u32)PAGE_SIZE);

	/* align block size to block offset */
	while (part_start & ((blocksize >> HFSPLUS_SECTOR_SHIFT) - 1))
		blocksize >>= 1;

	if (sb_set_blocksize(sb, blocksize) != blocksize) {
		printk(KERN_ERR "hfs: unable to set blocksize to %u!\n", blocksize);
		goto out_free_backup_vhdr;
	}

	HFSPLUS_SB(sb).blockoffset = part_start >>
			(sb->s_blocksize_bits - HFSPLUS_SECTOR_SHIFT);
	HFSPLUS_SB(sb).part_start = part_start;
	HFSPLUS_SB(sb).sect_count = part_size;
	HFSPLUS_SB(sb).fs_shift = HFSPLUS_SB(sb).alloc_blksz_shift -
			sb->s_blocksize_bits;

	bh = sb_bread512(sb, part_start + HFSPLUS_VOLHEAD_SECTOR, vhdr);
    if(!bh)
			goto out_free_backup_vhdr;

	HFSPLUS_SB(sb).s_vhbh = bh;
	HFSPLUS_SB(sb).s_vhdr = vhdr;
	return 0;

out_free_backup_vhdr:
//	if(HFSPLUS_SB(sb).s_backup_vhdr)
//		kfree(HFSPLUS_SB(sb).s_backup_vhdr);
out_free_vhdr:
//	if(HFSPLUS_SB(sb).s_vhdr)
//		kfree(HFSPLUS_SB(sb).s_vhdr);
out:
//	brelse(bh);
	return error;
}
