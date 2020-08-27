/*
 * Example ram filesystem kernel module for Linux.
 * This file is based on fs/ramfs kernel example.
 */
#include <linux/module.h>           /* For module macros */
#include <linux/fs.h>               /* For fs data */
#include <linux/pagemap.h>          /* For mapping */
#include <linux/parser.h>           /* For options parser */
#include <linux/slab.h>             /* For kernel alloc/free */
#include <linux/seq_file.h>         /* For seq_printf */

#define EXRAMFS_MAGIC         0xBAADBABE
#define EXRAMFS_DEFAULT_MODE  0755

/* Address space operations */
static int exramfs_set_page_dirty(struct page *page);

/* File operations */
static unsigned long exramfs_mmu_get_unmapped_area(struct file *file,
                                                   unsigned long addr,
                                                   unsigned long len,
                                                   unsigned long pgoff,
                                                   unsigned long flags);

/* Dir inode operations */
static int exramfs_create(struct inode *dir, struct dentry *dentry,
                          umode_t mode, bool excl);
static int exramfs_symlink(struct inode *dir, struct dentry *dentry,
                           const char *symname);
static int exramfs_mkdir(struct inode *dir, struct dentry *dentry,
                         umode_t mode);
static int exramfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
                         dev_t dev);


/* Address space operations */
static const struct address_space_operations exramfs_aops = {
    .readpage = simple_readpage,
    .write_begin = simple_write_begin,
    .write_end = simple_write_end,
    .set_page_dirty = exramfs_set_page_dirty,
};

/* File operations */
static const struct file_operations exramfs_fops = {
    .read_iter = generic_file_read_iter,
    .write_iter = generic_file_write_iter,
    .llseek = generic_file_llseek,
    .mmap = generic_file_mmap,
    .fsync = noop_fsync,
    .splice_read = generic_file_splice_read,
    .splice_write = iter_file_splice_write,
    .get_unmapped_area = exramfs_mmu_get_unmapped_area,
};

/* File inode operations */
static const struct inode_operations exramfs_file_iops = {
    .setattr = simple_setattr,
    .getattr = simple_getattr,
};

/* Dir inode operations */
static const struct inode_operations exramfs_dir_iops = {
    .create     = exramfs_create,
    .lookup     = simple_lookup,
    .link       = simple_link,
    .unlink     = simple_unlink,
    .symlink    = exramfs_symlink,
    .mkdir      = exramfs_mkdir,
    .rmdir      = simple_rmdir,
    .mknod      = exramfs_mknod,
    .rename     = simple_rename,
};

/* Superblock operations */
static const struct super_operations exramfs_ops = {
    .statfs = simple_statfs,
    .drop_inode = generic_delete_inode,
};

static unsigned long exramfs_mmu_get_unmapped_area(struct file *file,
                                                   unsigned long addr,
                                                   unsigned long len,
                                                   unsigned long pgoff,
                                                   unsigned long flags)
{
    return current->mm->get_unmapped_area(file, addr, len, pgoff, flags);
}

static int exramfs_set_page_dirty(struct page *page)
{
    if (!PageDirty(page)) {
        return !TestSetPageDirty(page);
    }

    return 0;
}

struct inode *exramfs_get_inode(struct super_block *sb, const struct inode *dir,
                                umode_t mode, dev_t dev)
{
    struct inode *inode = new_inode(sb);

    if (inode) {
        inode->i_ino = get_next_ino();
        inode_init_owner(inode, dir, mode);
        inode->i_mapping->a_ops = &exramfs_aops;
        mapping_set_gfp_mask(inode->i_mapping, GFP_HIGHUSER);
        mapping_set_unevictable(inode->i_mapping);
        inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
        switch (mode & S_IFMT) {
        default:
            init_special_inode(inode, mode, dev);
            break;
        case S_IFREG:
            inode->i_op = &exramfs_file_iops;
            inode->i_fop = &exramfs_fops;
            break;
        case S_IFDIR:
            inode->i_op = &exramfs_dir_iops;
            inode->i_fop = &simple_dir_operations;
            inc_nlink(inode);
            break;
        case S_IFLNK:
            inode->i_op = &page_symlink_inode_operations;
            break;
        }
    }

    return inode;
}

static int exramfs_mknod(struct inode *dir, struct dentry *dentry, umode_t mode,
                         dev_t dev)
{
    struct inode *inode = exramfs_get_inode(dir->i_sb, dir, mode, dev);
    int error = -ENOSPC;

    if (inode) {
        d_instantiate(dentry, inode);
        dget(dentry);
        error = 0;
        dir->i_mtime = dir->i_ctime = current_time(dir);
    }

    return error;
}

static int exramfs_mkdir(struct inode *dir, struct dentry *dentry, umode_t mode)
{
    int retval = exramfs_mknod(dir, dentry, mode | S_IFDIR, 0);
    
    if (!retval) {
        inc_nlink(dir);
    }

    return retval;
}

static int exramfs_create(struct inode *dir, struct dentry *dentry,
                          umode_t mode, bool excl)
{
    return exramfs_mknod(dir, dentry, mode | S_IFREG, 0);
}

static int exramfs_symlink(struct inode *dir, struct dentry *dentry,
                           const char *symname)
{
    struct inode *inode;
    int error = -ENOSPC;

    inode = exramfs_get_inode(dir->i_sb, dir, S_IFLNK | S_IRWXUGO, 0);
    if (inode) {
        int l = strlen(symname) + 1;
        error = page_symlink(inode, symname, l);
        if (!error) {
            d_instantiate(dentry, inode);
            dget(dentry);
            dir->i_mtime = dir->i_ctime = current_time(dir);
        } else {
            iput(inode);
        }
    }

    return error;
}

enum exramfs_param {
    Opt_mode,
    Opt_err,
};

static const match_table_t tokens = {
    {Opt_mode, "mode=%o"},
    {Opt_err, NULL},
};

static int exramfs_parse_options(char *data, umode_t *mode)
{
    substring_t args[MAX_OPT_ARGS];
    int option;
    int token;
    char *p;

    *mode = EXRAMFS_DEFAULT_MODE;

    while ((p = strsep(&data, ",")) != NULL) {
        if (!*p) {
            continue;
        }

        token = match_token(p, tokens, args);
        switch (token) {
        case Opt_mode:
            if (match_octal(&args[0], &option)) {
                return -EINVAL;
            }
            *mode = option & S_IALLUGO;
            break;
        }
    }

    return 0;
}

int exramfs_fill_super(struct super_block *sb, void *data, int silent)
{
    umode_t *mode;
    struct inode *inode;
    int err;

    mode = kzalloc(sizeof(umode_t), GFP_KERNEL);
    sb->s_fs_info = mode;
    if (!mode) {
        return -ENOMEM;
    }

    err = exramfs_parse_options(data, mode);
    if (err) {
        return err;
    }

    sb->s_maxbytes = MAX_LFS_FILESIZE;
    sb->s_blocksize = PAGE_SIZE;
    sb->s_blocksize_bits = PAGE_SHIFT;
    sb->s_magic = EXRAMFS_MAGIC;
    sb->s_op = &exramfs_ops;
    sb->s_time_gran = 1;

    inode = exramfs_get_inode(sb, NULL, S_IFDIR | *mode, 0);
    sb->s_root = d_make_root(inode);
    if (!sb->s_root) {
        return -ENOMEM;
    }

    return 0;
}

struct dentry *exramfs_mount(struct file_system_type *fs_type, int flags,
                             const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, data, exramfs_fill_super);
}

static void exramfs_kill_sb(struct super_block *sb)
{
    kfree(sb->s_fs_info);
    kill_litter_super(sb);
}

static struct file_system_type exramfs_fs_type = {
    .name       = "exramfs",
    .mount      = exramfs_mount,
    .kill_sb    = exramfs_kill_sb,
    .fs_flags   = FS_USERNS_MOUNT,
};

static int __init init_exramfs(void)
{
    printk(KERN_INFO "insert exramfs\n");
    return register_filesystem(&exramfs_fs_type);
}

static void __exit exit_exramfs(void)
{
    printk(KERN_INFO "remove exramfs\n");
    unregister_filesystem(&exramfs_fs_type);
}

MODULE_LICENSE("GPL");
module_init(init_exramfs);
module_exit(exit_exramfs);
