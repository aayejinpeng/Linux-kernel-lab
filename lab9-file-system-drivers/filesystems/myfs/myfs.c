/*
 * SO2 Lab - Filesystem drivers
 * Exercise #1 (no-dev filesystem)
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/pagemap.h>

MODULE_DESCRIPTION("Simple no-dev filesystem");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define MYFS_BLOCKSIZE		4096
#define MYFS_BLOCKSIZE_BITS	12
#define MYFS_MAGIC		0xbeefcafe
#define LOG_LEVEL		KERN_ALERT

/* declarations of functions that are part of operation structures */

static int myfs_mknod(struct inode *dir,
		struct dentry *dentry, umode_t mode, dev_t dev);
static int myfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl);
static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode);

/* TODO 2: define super_operations structure */
static const struct super_operations myfs_ops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
        //.show_options   = ramfs_show_options,
};
static const struct inode_operations myfs_dir_inode_operations = {
	/* TODO 5: Fill dir inode operations structure. */
	.create = myfs_create,
	.lookup = simple_lookup,
	.link = simple_link,
	.unlink = simple_unlink,
	.mkdir = myfs_mkdir,
	.rmdir = simple_rmdir,
	.mknod = myfs_mknod,
	.rename = simple_rename,
};

static const struct file_operations myfs_file_operations = {
	/* TODO 6: Fill file operations structure. */
	.read_iter	= generic_file_read_iter,
	.write_iter	= generic_file_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= noop_fsync,
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.llseek		= generic_file_llseek,
};

static const struct inode_operations myfs_file_inode_operations = {
	/* TODO 6: Fill file inode operations structure. */
	.setattr	= simple_setattr,
	.getattr	= simple_getattr,
};

static const struct address_space_operations myfs_aops = {
	/* TODO 6: Fill address space operations structure. */
	.readpage	= simple_readpage,
	.write_begin	= simple_write_begin,
	.write_end	= simple_write_end,
};

struct inode *myfs_get_inode(struct super_block *sb, const struct inode *dir,
		int mode)
{
	struct inode *inode = new_inode(sb);

	if (!inode)
		return NULL;

	/* TODO 3: fill inode structure
	 *     - mode
	 *     - uid
	 *     - gid
	 *     - atime,ctime,mtime
	 *     - ino
	 */
	inode_init_owner(inode, dir, mode);
	inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
	/* TODO 5: Init i_ino using get_next_ino */
	inode->i_ino = get_next_ino();
	/* TODO 6: Initialize address space operations. */
	inode->i_op = &myfs_file_inode_operations;
	inode->i_fop = &myfs_file_operations;
	inode->i_mapping->a_ops = &myfs_aops;
	if (S_ISDIR(mode)) {
		/* TODO 3: set inode operations for dir inodes. */
		inode->i_op = &simple_dir_inode_operations;
		inode->i_fop = &simple_dir_operations;
		/* TODO 5: use myfs_dir_inode_operations for inode
		 * operations (i_op).
		 */
		inode->i_op = &myfs_dir_inode_operations;
		/* TODO 3: directory inodes start off with i_nlink == 2 (for "." entry).
		 * Directory link count should be incremented (use inc_nlink).
		 */
		inc_nlink(inode);
	}

	/* TODO 6: Set file inode and file operations for regular files
	 * (use the S_ISREG macro).
	 */

	return inode;
}

/* TODO 5: Implement myfs_mknod, myfs_create, myfs_mkdir. */
static int myfs_mknod(struct inode *dir,
		struct dentry *dentry, umode_t mode, dev_t dev)
{
	struct inode * inode = myfs_get_inode(dir->i_sb, dir, mode);
	int error = -ENOSPC;

	if (inode) {
		d_instantiate(dentry, inode);
		dget(dentry);	/* Extra count - pin the dentry in core */
		error = 0;
		dir->i_mtime = dir->i_ctime = current_time(dir);
	}
	return error;
}

static int myfs_create(struct inode *dir, struct dentry *dentry,
		umode_t mode, bool excl)
{
	return myfs_mknod(dir, dentry, mode | S_IFREG, 0);
}
static int myfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int retval = myfs_mknod(dir, dentry, mode | S_IFDIR, 0);
	if (!retval)
		inc_nlink(dir);
	return retval;
}

static int myfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode;
	struct dentry *root_dentry;

	/* TODO 2: fill super_block
	 *   - blocksize, blocksize_bits
	 *   - magic
	 *   - super operations
	 *   - maxbytes MAX_LFS_FILESIZE
	 */
	sb->s_maxbytes          = ((loff_t)ULONG_MAX<<MYFS_BLOCKSIZE_BITS);
	sb->s_blocksize         = MYFS_BLOCKSIZE;
	sb->s_blocksize_bits    = MYFS_BLOCKSIZE_BITS;
	sb->s_magic             = MYFS_MAGIC;
	sb->s_op                = &myfs_ops;
	//sb->s_time_gran         = 1;

	/* mode = directory & access rights (755) */
	root_inode = myfs_get_inode(sb, NULL,
			S_IFDIR | S_IRWXU | S_IRGRP |
			S_IXGRP | S_IROTH | S_IXOTH);

	printk(LOG_LEVEL "root inode has %d link(s)\n", root_inode->i_nlink);

	if (!root_inode)
		return -ENOMEM;

	root_dentry = d_make_root(root_inode);
	if (!root_dentry)
		goto out_no_root;
	sb->s_root = root_dentry;

	return 0;

out_no_root:
	iput(root_inode);
	return -ENOMEM;
}

static struct dentry *myfs_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data)
{
	/* TODO 1: call superblock mount function */
	//文件系统在挂载时会调用的函数，mount_nodev是一个函数模板了，对应于没有物理设备的文件系统，我们
	//需要提供一个super_block填充的函数即可，mount_nodev内部实现会做好alloc_super等数据结构的申请
	return mount_nodev(fs_type,flags,data,myfs_fill_super);
}

/* TODO 1: define file_system_type structure */
static struct file_system_type my_fs_type = {
		.owner			= THIS_MODULE,
        .name           = "myfs",
        .mount          = myfs_mount,
        .kill_sb        = kill_litter_super,
        .fs_flags       = FS_USERNS_MOUNT,
};
static int __init myfs_init(void)
{
	int err;

	/* TODO 1: register */
	err = register_filesystem(&my_fs_type);
	if (err) {
		printk(LOG_LEVEL "register_filesystem failed\n");
		return err;
	}

	return 0;
}

static void __exit myfs_exit(void)
{
	/* TODO 1: unregister */
	unregister_filesystem(&my_fs_type);
}

module_init(myfs_init);
module_exit(myfs_exit);
