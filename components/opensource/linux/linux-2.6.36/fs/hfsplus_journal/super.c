/*
 *  linux/fs/hfsplus/super.c
 *
 * Copyright (C) 2001
 * Brad Boyer (flar@allandria.com)
 * (C) 2003 Ardis Technologies <roman@ardistech.com>
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pagemap.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/vfs.h>
#include <linux/nls.h>

static struct inode *hfsplus_alloc_inode(struct super_block *sb);
static void hfsplus_destroy_inode(struct inode *inode);
static struct kmem_cache *hfsplus_inode_cachep;


#include "hfsplus_fs.h"

struct inode *hfsplus_iget(struct super_block *sb, unsigned long ino)
{
	struct hfs_find_data fd;
	struct hfsplus_vh *vhdr;
	hfsplus_handle_t hfsplus_handle;

	struct inode *inode;
	long err = -EIO;

	inode = iget_locked(sb, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;

	INIT_LIST_HEAD(&HFSPLUS_I(inode).open_dir_list);
	mutex_init(&HFSPLUS_I(inode).extents_lock);
	HFSPLUS_I(inode).flags = 0;
	HFSPLUS_I(inode).rsrc_inode = NULL;
	atomic_set(&HFSPLUS_I(inode).opencnt, 0);

	if (inode->i_ino >= HFSPLUS_FIRSTUSER_CNID) {
	read_inode:
		hfs_find_init(HFSPLUS_SB(inode->i_sb).cat_tree, &fd);
		if (hfsplus_journal_start(__FUNCTION__, inode->i_sb, &hfsplus_handle))
			goto bad_inode;
		err = hfsplus_find_cat(&hfsplus_handle, inode->i_sb, inode->i_ino, &fd);
		if (!err)
			err = hfsplus_cat_read_inode(inode, &fd);
		hfs_find_exit(&hfsplus_handle, &fd);
		hfsplus_journal_stop(&hfsplus_handle);
		if (err)
			goto bad_inode;
		goto done;
	}
	vhdr = HFSPLUS_SB(inode->i_sb).s_vhdr;
	switch(inode->i_ino) {
	case HFSPLUS_ROOT_CNID:
		goto read_inode;
	case HFSPLUS_EXT_CNID:
		hfsplus_inode_read_fork(inode, &vhdr->ext_file);
		if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_JOURNALED))
			inode->i_mapping->a_ops = &hfsplus_journalled_btree_aops;
		else
			inode->i_mapping->a_ops = &hfsplus_btree_aops;
		break;
	case HFSPLUS_CAT_CNID:
		hfsplus_inode_read_fork(inode, &vhdr->cat_file);
		if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_JOURNALED))
			inode->i_mapping->a_ops = &hfsplus_journalled_btree_aops;
		else
			inode->i_mapping->a_ops = &hfsplus_btree_aops;
		break;
	case HFSPLUS_ALLOC_CNID:
		hfsplus_inode_read_fork(inode, &vhdr->alloc_file);
		if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_JOURNALED))
			inode->i_mapping->a_ops = &hfsplus_journalled_aops;
		else
			inode->i_mapping->a_ops = &hfsplus_aops;
		break;
	case HFSPLUS_START_CNID:
		hfsplus_inode_read_fork(inode, &vhdr->start_file);
		break;
	case HFSPLUS_ATTR_CNID:
		hfsplus_inode_read_fork(inode, &vhdr->attr_file);
		if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_JOURNALED))
			inode->i_mapping->a_ops = &hfsplus_journalled_btree_aops;
		else
			inode->i_mapping->a_ops = &hfsplus_btree_aops;
		break;
	default:
		goto bad_inode;
	}

done:
	unlock_new_inode(inode);
	return inode;

bad_inode:
	iget_failed(inode);
	return ERR_PTR(err);
}

static int hfsplus_write_inode(struct inode *inode,
		struct writeback_control *wbc)
{
	struct hfsplus_vh *vhdr;
	hfsplus_handle_t hfsplus_handle;
	int ret = 0;

	dprint(DBG_INODE, "hfsplus_write_inode: %lu\n", inode->i_ino);
	hfsplus_handle.journaled = !HFSPLUS_JOURNAL_PRESENT;

	hfsplus_ext_write_extent(&hfsplus_handle, inode);
	if (inode->i_ino >= HFSPLUS_FIRSTUSER_CNID) {
		ret = hfsplus_cat_write_inode(&hfsplus_handle, inode);
		return ret;
	}
	vhdr = HFSPLUS_SB(inode->i_sb).s_vhdr;
	switch (inode->i_ino) {
	case HFSPLUS_ROOT_CNID:
		ret = hfsplus_cat_write_inode(&hfsplus_handle, inode);
		break;
	case HFSPLUS_EXT_CNID:
		if (vhdr->ext_file.total_size != cpu_to_be64(inode->i_size)) {
			HFSPLUS_SB(inode->i_sb).flags |= HFSPLUS_SB_WRITEBACKUP;
			inode->i_sb->s_dirt = 1;
		}
		hfsplus_inode_write_fork(inode, &vhdr->ext_file);
		hfs_btree_write(&hfsplus_handle, HFSPLUS_SB(inode->i_sb).ext_tree);
		break;
	case HFSPLUS_CAT_CNID:
		if (vhdr->cat_file.total_size != cpu_to_be64(inode->i_size)) {
			HFSPLUS_SB(inode->i_sb).flags |= HFSPLUS_SB_WRITEBACKUP;
			inode->i_sb->s_dirt = 1;
		}
		hfsplus_inode_write_fork(inode, &vhdr->cat_file);
		hfs_btree_write(&hfsplus_handle, HFSPLUS_SB(inode->i_sb).cat_tree);
		break;
	case HFSPLUS_ALLOC_CNID:
		if (vhdr->alloc_file.total_size != cpu_to_be64(inode->i_size)) {
			HFSPLUS_SB(inode->i_sb).flags |= HFSPLUS_SB_WRITEBACKUP;
			inode->i_sb->s_dirt = 1;
		}
		hfsplus_inode_write_fork(inode, &vhdr->alloc_file);
		break;
	case HFSPLUS_START_CNID:
		if (vhdr->start_file.total_size != cpu_to_be64(inode->i_size)) {
			HFSPLUS_SB(inode->i_sb).flags |= HFSPLUS_SB_WRITEBACKUP;
			inode->i_sb->s_dirt = 1;
		}
		hfsplus_inode_write_fork(inode, &vhdr->start_file);
		break;
	case HFSPLUS_ATTR_CNID:
		if (vhdr->attr_file.total_size != cpu_to_be64(inode->i_size)) {
			HFSPLUS_SB(inode->i_sb).flags |= HFSPLUS_SB_WRITEBACKUP;
			inode->i_sb->s_dirt = 1;
		}
		hfsplus_inode_write_fork(inode, &vhdr->attr_file);
		hfs_btree_write(&hfsplus_handle, HFSPLUS_SB(inode->i_sb).attr_tree);
		break;
	}
	return ret;
}

static void hfsplus_evict_inode(struct inode *inode)
{
	dprint(DBG_INODE, "hfsplus_evict_inode: %lu\n", inode->i_ino);
	truncate_inode_pages(&inode->i_data, 0);
	end_writeback(inode);
	if (HFSPLUS_IS_RSRC(inode)) {
		HFSPLUS_I(HFSPLUS_I(inode).rsrc_inode).rsrc_inode = NULL;
		iput(HFSPLUS_I(inode).rsrc_inode);
	}
}

int hfsplus_sync_fs(struct super_block *sb, int wait)
{
	struct hfsplus_vh *vhdr = HFSPLUS_SB(sb).s_vhdr;
	hfsplus_handle_t hfsplus_handle;

	dprint(DBG_SUPER, "hfsplus_write_super\n");

	lock_super(sb);
	sb->s_dirt = 0;
	if (sb->s_flags & MS_RDONLY)
		/* warn? */
		return -1;

	if (hfsplus_journal_start(__FUNCTION__, sb, &hfsplus_handle))
		return -1;

	vhdr->free_blocks = cpu_to_be32(HFSPLUS_SB(sb).free_blocks);
	vhdr->next_alloc = cpu_to_be32(HFSPLUS_SB(sb).next_alloc);
	vhdr->next_cnid = cpu_to_be32(HFSPLUS_SB(sb).next_cnid);
	vhdr->folder_count = cpu_to_be32(HFSPLUS_SB(sb).folder_count);
	vhdr->file_count = cpu_to_be32(HFSPLUS_SB(sb).file_count);

	if (hfsplus_handle.journaled == HFSPLUS_JOURNAL_PRESENT) {
		int err;

		BUG_ON(!buffer_mapped(HFSPLUS_SB(sb).s_vhbh));

		err = hfsplus_journal_get_write_access(__FUNCTION__, &hfsplus_handle, HFSPLUS_SB(sb).s_vhbh);
		if (err) {
			printk(KERN_ERR "%s: Error in journal_get_write_access\n", __FUNCTION__);
			goto write_super_out;
		}
		err = hfsplus_jbd_dirty_metadata(hfsplus_handle.handle, HFSPLUS_SB(sb).s_vhbh);
		if (err) {
			printk(KERN_ERR "%s: Error in hfsplus_jbd_dirty_metadata\n", __FUNCTION__);
			goto write_super_out;
		}
	}
	else
	mark_buffer_dirty(HFSPLUS_SB(sb).s_vhbh);
	if (HFSPLUS_SB(sb).flags & HFSPLUS_SB_WRITEBACKUP) {
		if (HFSPLUS_SB(sb).sect_count) {
			struct buffer_head *bh;
			u32 block, offset;

			block = HFSPLUS_SB(sb).blockoffset;
			block += (HFSPLUS_SB(sb).sect_count - 2) >> (sb->s_blocksize_bits - 9);
			offset = ((HFSPLUS_SB(sb).sect_count - 2) << 9) & (sb->s_blocksize - 1);
			printk(KERN_DEBUG "hfs: backup: %u,%u,%u,%u\n", HFSPLUS_SB(sb).blockoffset,
				HFSPLUS_SB(sb).sect_count, block, offset);
			bh = sb_bread(sb, block);
			if (bh) {
				vhdr = (struct hfsplus_vh *)(bh->b_data + offset);
				if (be16_to_cpu(vhdr->signature) == HFSPLUS_VOLHEAD_SIG) {
					memcpy(vhdr, HFSPLUS_SB(sb).s_vhdr, sizeof(*vhdr));
					if (hfsplus_handle.journaled == HFSPLUS_JOURNAL_PRESENT)
						hfsplus_jbd_dirty_metadata(hfsplus_handle.handle, bh);
					else
					mark_buffer_dirty(bh);
					brelse(bh);
				} else
					printk(KERN_WARNING "hfs: backup not found!\n");
			}
		}
		HFSPLUS_SB(sb).flags &= ~HFSPLUS_SB_WRITEBACKUP;
	}
write_super_out:

	unlock_super(sb);
		hfsplus_journal_stop(&hfsplus_handle);

	return 0;
}

static void hfsplus_write_super(struct super_block *sb)
{
	if (!(sb->s_flags & MS_RDONLY))
		hfsplus_sync_fs(sb, 1);
	else
		sb->s_dirt = 0;
}

static void hfsplus_put_super(struct super_block *sb)
{
	dprint(DBG_SUPER, "hfsplus_put_super\n");
	if (!sb->s_fs_info)
		return;

	lock_kernel();
	hfsplus_journalled_deinit(sb);

	if (sb->s_dirt)
		hfsplus_write_super(sb);
	if (!(sb->s_flags & MS_RDONLY) && HFSPLUS_SB(sb).s_vhdr) {
		struct hfsplus_vh *vhdr = HFSPLUS_SB(sb).s_vhdr;

		vhdr->modify_date = hfsp_now2mt();
		vhdr->attributes |= cpu_to_be32(HFSPLUS_VOL_UNMNT);
		vhdr->attributes &= cpu_to_be32(~HFSPLUS_VOL_INCNSTNT);
		mark_buffer_dirty(HFSPLUS_SB(sb).s_vhbh);
		sync_dirty_buffer(HFSPLUS_SB(sb).s_vhbh);
	}

	hfs_btree_close(HFSPLUS_SB(sb).cat_tree);
	hfs_btree_close(HFSPLUS_SB(sb).ext_tree);
	iput(HFSPLUS_SB(sb).alloc_file);
	iput(HFSPLUS_SB(sb).hidden_dir);
	brelse(HFSPLUS_SB(sb).s_vhbh);
	unload_nls(HFSPLUS_SB(sb).nls);
	kfree(sb->s_fs_info);
	sb->s_fs_info = NULL;

	unlock_kernel();
}

static int hfsplus_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	struct super_block *sb = dentry->d_sb;
	u64 id = huge_encode_dev(sb->s_bdev->bd_dev);

	buf->f_type = HFSPLUS_SUPER_MAGIC;
	buf->f_bsize = sb->s_blocksize;
	buf->f_blocks = HFSPLUS_SB(sb).total_blocks << HFSPLUS_SB(sb).fs_shift;
	buf->f_bfree = HFSPLUS_SB(sb).free_blocks << HFSPLUS_SB(sb).fs_shift;
	buf->f_bavail = buf->f_bfree;
	buf->f_files = 0xFFFFFFFF;
	buf->f_ffree = 0xFFFFFFFF - HFSPLUS_SB(sb).next_cnid;
	buf->f_fsid.val[0] = (u32)id;
	buf->f_fsid.val[1] = (u32)(id >> 32);
	buf->f_namelen = HFSPLUS_MAX_STRLEN;

	return 0;
}

static int hfsplus_remount(struct super_block *sb, int *flags, char *data)
{
	if ((*flags & MS_RDONLY) == (sb->s_flags & MS_RDONLY))
		return 0;
	if (!(*flags & MS_RDONLY)) {
		struct hfsplus_vh *vhdr = HFSPLUS_SB(sb).s_vhdr;
		struct hfsplus_sb_info sbi;

		memset(&sbi, 0, sizeof(struct hfsplus_sb_info));
		sbi.nls = HFSPLUS_SB(sb).nls;
		if (!hfsplus_parse_options(data, &sbi))
			return -EINVAL;

		if (!(vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_UNMNT))) {
			printk(KERN_WARNING "hfs: filesystem was not cleanly unmounted, "
			       "running fsck.hfsplus is recommended.  leaving read-only.\n");
			sb->s_flags |= MS_RDONLY;
			*flags |= MS_RDONLY;
		} else if (sbi.flags & HFSPLUS_SB_FORCE) {
			/* nothing */
		} else if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_SOFTLOCK)) {
			printk(KERN_WARNING "hfs: filesystem is marked locked, leaving read-only.\n");
			sb->s_flags |= MS_RDONLY;
			*flags |= MS_RDONLY;
		} else if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_JOURNALED)) {
			if (hfsplus_journalled_check(sb)) {
			printk(KERN_WARNING "hfs: filesystem is marked journaled, leaving read-only.\n");
			sb->s_flags |= MS_RDONLY;
			*flags |= MS_RDONLY;
			} else
				printk("HFS+-fs: Able to mount journaled hfsplus volume in read-write mode\n");
		}
	}
	return 0;
}

static const struct super_operations hfsplus_sops = {
	.alloc_inode	= hfsplus_alloc_inode,
	.destroy_inode	= hfsplus_destroy_inode,
	.write_inode	= hfsplus_write_inode,
	.evict_inode	= hfsplus_evict_inode,
	.put_super	= hfsplus_put_super,
	.write_super	= hfsplus_write_super,
	.sync_fs	= hfsplus_sync_fs,
	.statfs		= hfsplus_statfs,
	.remount_fs	= hfsplus_remount,
	.show_options	= hfsplus_show_options,
};

static int hfsplus_fill_super(struct super_block *sb, void *data, int silent)
{
	struct hfsplus_vh *vhdr;
	struct hfsplus_sb_info *sbi;
	hfsplus_cat_entry entry;
	struct hfs_find_data fd;
	struct inode *root, *inode;
	struct qstr str;
	struct nls_table *nls = NULL;
	hfsplus_handle_t hfsplus_handle;
	int err = -EINVAL;

	sbi = kzalloc(sizeof(*sbi), GFP_KERNEL);
	if (!sbi)
		return -ENOMEM;

	sb->s_fs_info = sbi;
	INIT_HLIST_HEAD(&sbi->rsrc_inodes);
	hfsplus_fill_defaults(sbi);
	if (!hfsplus_parse_options(data, sbi)) {
		printk(KERN_ERR "hfs: unable to parse mount options\n");
		err = -EINVAL;
		goto cleanup;
	}

	/* temporarily use utf8 to correctly find the hidden dir below */
	nls = sbi->nls;
	sbi->nls = load_nls("utf8");
	if (!sbi->nls) {
		printk(KERN_ERR "hfs: unable to load nls for utf8\n");
		err = -EINVAL;
		goto cleanup;
	}

	/* Grab the volume header */
	if (hfsplus_read_wrapper(sb)) {
		if (!silent)
			printk(KERN_WARNING "hfs: unable to find HFS+ superblock\n");
		err = -EINVAL;
		goto cleanup;
	}
	vhdr = HFSPLUS_SB(sb).s_vhdr;

	/* Copy parts of the volume header into the superblock */
	sb->s_magic = HFSPLUS_VOLHEAD_SIG;
	if (be16_to_cpu(vhdr->version) < HFSPLUS_MIN_VERSION ||
	    be16_to_cpu(vhdr->version) > HFSPLUS_CURRENT_VERSION) {
		printk(KERN_ERR "hfs: wrong filesystem version\n");
		goto cleanup;
	}

	hfsplus_journalled_init(sb, vhdr);
	HFSPLUS_SB(sb).jnl.s_journal = NULL;
	if (HFSPLUS_SB(sb).jnl.journaled == HFSPLUS_JOURNAL_PRESENT) {
		if (hfsplus_journalled_check(sb)) {
			if (!silent)
				printk("HFS+-fs: Error in journal, use the force option at your own risk, mounting read-only.\n");
			if (HFSPLUS_SB(sb).s_vhdr == NULL) {
				printk("HFS+-fs: Error in Volume Header\n");
				goto cleanup;
			}
			sb->s_flags |= MS_RDONLY;
		} else {
			hfsplus_jbd_t *journal;

			journal = hfsplus_get_dev_journal(sb);
			if (journal == NULL) {
				printk("HFS+-fs: Error in getting journal device from JBD layer\n");
				sb->s_flags |= MS_RDONLY;
			} else {
				int err;
				err = hfsplus_jbd_load(journal);
				if (err == 0) {
					dprint(DBG_JOURNAL, "HFS+-fs: Successfully load journal\n");
					HFSPLUS_SB(sb).jnl.s_journal = journal;
				} else {
					printk("HFS+-fs: Error in loading journal\n");
					hfsplus_jbd_destroy(journal);
					sb->s_flags |= MS_RDONLY;
				}
			}
			if (sb->s_flags & MS_RDONLY)
				printk("HFS+-fs: Error in journal, mounting read-only.\n");
			else
				dprint(DBG_JOURNAL, "HFS+-fs: No problem in journal. Should be able to mount hfsplus volume in read-write mode\n");
		}
	}

	HFSPLUS_SB(sb).total_blocks = be32_to_cpu(vhdr->total_blocks);
	HFSPLUS_SB(sb).free_blocks = be32_to_cpu(vhdr->free_blocks);
	HFSPLUS_SB(sb).next_alloc = be32_to_cpu(vhdr->next_alloc);
	HFSPLUS_SB(sb).next_cnid = be32_to_cpu(vhdr->next_cnid);
	HFSPLUS_SB(sb).file_count = be32_to_cpu(vhdr->file_count);
	HFSPLUS_SB(sb).folder_count = be32_to_cpu(vhdr->folder_count);
	HFSPLUS_SB(sb).data_clump_blocks = be32_to_cpu(vhdr->data_clump_sz) >> HFSPLUS_SB(sb).alloc_blksz_shift;
	if (!HFSPLUS_SB(sb).data_clump_blocks)
		HFSPLUS_SB(sb).data_clump_blocks = 1;
	HFSPLUS_SB(sb).rsrc_clump_blocks = be32_to_cpu(vhdr->rsrc_clump_sz) >> HFSPLUS_SB(sb).alloc_blksz_shift;
	if (!HFSPLUS_SB(sb).rsrc_clump_blocks)
		HFSPLUS_SB(sb).rsrc_clump_blocks = 1;

	/* Set up operations so we can load metadata */
	sb->s_op = &hfsplus_sops;
	sb->s_maxbytes = MAX_LFS_FILESIZE;

	if (!(vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_UNMNT))) {
		printk(KERN_WARNING "hfs: Filesystem was not cleanly unmounted, "
		       "running fsck.hfsplus is recommended.  mounting read-only.\n");
        /* Foxconn removed start pling 05/31/2010 */
        /* Ignore this flag to force writeable */
#if 0
		sb->s_flags |= MS_RDONLY;
#endif
        /* Foxconn removed end pling 05/31/2010 */
	} else if (sbi->flags & HFSPLUS_SB_FORCE) {
		/* nothing */
	} else if (vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_SOFTLOCK)) {
		printk(KERN_WARNING "hfs: Filesystem is marked locked, mounting read-only.\n");
		sb->s_flags |= MS_RDONLY;
	} else if ((vhdr->attributes & cpu_to_be32(HFSPLUS_VOL_JOURNALED)) && !(sb->s_flags & MS_RDONLY)) {
		printk(KERN_WARNING "hfs: write access to a journaled filesystem is not supported, "
		       "use the force option at your own risk, mounting read-only.\n");
		sb->s_flags |= MS_RDONLY;
	}
	sbi->flags &= ~HFSPLUS_SB_FORCE;

	/* Load metadata objects (B*Trees) */
	HFSPLUS_SB(sb).ext_tree = hfs_btree_open(sb, HFSPLUS_EXT_CNID);
	if (!HFSPLUS_SB(sb).ext_tree) {
		printk(KERN_ERR "hfs: failed to load extents file\n");
		goto cleanup;
	}
	HFSPLUS_SB(sb).cat_tree = hfs_btree_open(sb, HFSPLUS_CAT_CNID);
	if (!HFSPLUS_SB(sb).cat_tree) {
		printk(KERN_ERR "hfs: failed to load catalog file\n");
		goto cleanup;
	}

	inode = hfsplus_iget(sb, HFSPLUS_ALLOC_CNID);
	if (IS_ERR(inode)) {
		printk(KERN_ERR "hfs: failed to load allocation file\n");
		err = PTR_ERR(inode);
		goto cleanup;
	}
	HFSPLUS_SB(sb).alloc_file = inode;

	/* Load the root directory */
	root = hfsplus_iget(sb, HFSPLUS_ROOT_CNID);
	if (IS_ERR(root)) {
		printk(KERN_ERR "hfs: failed to load root directory\n");
		err = PTR_ERR(root);
		goto cleanup;
	}
	sb->s_root = d_alloc_root(root);
	if (!sb->s_root) {
		iput(root);
		err = -ENOMEM;
		goto cleanup;
	}
	sb->s_root->d_op = &hfsplus_dentry_operations;

	str.len = sizeof(HFSP_HIDDENDIR_NAME) - 1;
	str.name = HFSP_HIDDENDIR_NAME;
	hfs_find_init(HFSPLUS_SB(sb).cat_tree, &fd);
	hfsplus_cat_build_key(sb, fd.search_key, HFSPLUS_ROOT_CNID, &str);
	if (hfsplus_journal_start(__FUNCTION__, sb, &hfsplus_handle))
		goto cleanup;
	if (!hfs_brec_read(&hfsplus_handle, &fd, &entry, sizeof(entry))) { 
		hfs_find_exit(&hfsplus_handle, &fd);
		if (entry.type != cpu_to_be16(HFSPLUS_FOLDER)) {
			hfsplus_journal_stop(&hfsplus_handle);
			goto cleanup;
		}
		inode = hfsplus_iget(sb, be32_to_cpu(entry.folder.id));
		if (IS_ERR(inode)) {
					hfsplus_journal_stop(&hfsplus_handle);

			err = PTR_ERR(inode);
			goto cleanup;
		}
		HFSPLUS_SB(sb).hidden_dir = inode;
				hfsplus_journal_stop(&hfsplus_handle);

	} else {
		hfsplus_journal_stop(&hfsplus_handle);
		hfs_find_exit(NULL, &fd);
	}

	if (sb->s_flags & MS_RDONLY)
		goto out;

	/* H+LX == hfsplusutils, H+Lx == this driver, H+lx is unused
	 * all three are registered with Apple for our use
	 */
	if (HFSPLUS_SB(sb).jnl.journaled == HFSPLUS_JOURNAL_PRESENT)
		vhdr->last_mount_vers = cpu_to_be32(HFSP_MOUNT_JOURNALED_VERSION);
	else
		vhdr->last_mount_vers = cpu_to_be32(HFSP_MOUNT_VERSION);
	vhdr->modify_date = hfsp_now2mt();
	be32_add_cpu(&vhdr->write_count, 1);
	vhdr->attributes &= cpu_to_be32(~HFSPLUS_VOL_UNMNT);
	vhdr->attributes |= cpu_to_be32(HFSPLUS_VOL_INCNSTNT);
	mark_buffer_dirty(HFSPLUS_SB(sb).s_vhbh);
	sync_dirty_buffer(HFSPLUS_SB(sb).s_vhbh);

	if (!HFSPLUS_SB(sb).hidden_dir) {
		printk(KERN_DEBUG "hfs: create hidden dir...\n");
		if (hfsplus_journal_start(__FUNCTION__, sb, &hfsplus_handle))
			goto out;
		HFSPLUS_SB(sb).hidden_dir = hfsplus_new_inode(&hfsplus_handle, sb, S_IFDIR);
		hfsplus_create_cat(&hfsplus_handle, HFSPLUS_SB(sb).hidden_dir->i_ino, sb->s_root->d_inode,
				   &str, HFSPLUS_SB(sb).hidden_dir);
		hfsplus_journalled_mark_inode_dirty(__FUNCTION__, &hfsplus_handle, HFSPLUS_SB(sb).hidden_dir);
		hfsplus_journal_stop(&hfsplus_handle);
	}
out:
	unload_nls(sbi->nls);
	sbi->nls = nls;
	return 0;

cleanup:
	if (HFSPLUS_SB(sb).jnl.s_journal != NULL)
		HFSPLUS_SB(sb).jnl.s_journal->j_flags |= JFS_ABORT;
	hfsplus_put_super(sb);
	unload_nls(nls);
	return err;
}


static struct inode *hfsplus_alloc_inode(struct super_block *sb)
{
	struct hfsplus_inode_info *i;

	i = kmem_cache_alloc(hfsplus_inode_cachep, GFP_KERNEL);
	return i ? &i->vfs_inode : NULL;
}

static void hfsplus_destroy_inode(struct inode *inode)
{
	kmem_cache_free(hfsplus_inode_cachep, &HFSPLUS_I(inode));
}

#define HFSPLUS_INODE_SIZE	sizeof(struct hfsplus_inode_info)

static int hfsplus_get_sb(struct file_system_type *fs_type,
			  int flags, const char *dev_name, void *data,
			  struct vfsmount *mnt)
{
	return get_sb_bdev(fs_type, flags, dev_name, data, hfsplus_fill_super,
			   mnt);
}

static struct file_system_type hfsplus_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "hfsplus",
	.get_sb		= hfsplus_get_sb,
	.kill_sb	= kill_block_super,
	.fs_flags	= FS_REQUIRES_DEV,
};

static void hfsplus_init_once(void *p, struct kmem_cache *cachep, unsigned long flags)
{
	struct hfsplus_inode_info *i = p;

	inode_init_once(&i->vfs_inode);
}

static int __init init_hfsplus_fs(void)
{
	int err;

	hfsplus_inode_cachep = kmem_cache_create("hfsplus_icache",
		HFSPLUS_INODE_SIZE, 0, SLAB_HWCACHE_ALIGN,
		hfsplus_init_once);
	if (!hfsplus_inode_cachep)
		return -ENOMEM;
	err = hfsplus_jbd_init();
	if (err) {
		printk(KERN_ERR "Error in initializing hfsplus-jbd caches\n");
		kmem_cache_destroy(hfsplus_inode_cachep);
		return -ENOMEM;
	}
	err = register_filesystem(&hfsplus_fs_type);
	if (err) {
		hfsplus_jbd_exit();
		kmem_cache_destroy(hfsplus_inode_cachep);
	}
	return err;
}

static void __exit exit_hfsplus_fs(void)
{
	unregister_filesystem(&hfsplus_fs_type);
	hfsplus_jbd_exit();
	kmem_cache_destroy(hfsplus_inode_cachep);
}

MODULE_AUTHOR("Brad Boyer");
MODULE_DESCRIPTION("Extended Macintosh Filesystem");
MODULE_LICENSE("GPL");
module_init(init_hfsplus_fs)
module_exit(exit_hfsplus_fs)
