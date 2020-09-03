/*
 * Character device kernel module for Linux.
 * This file is based on fs/ramfs kernel example.
 */
#include <linux/device.h>           /* Device related functions*/
#include <linux/fs.h>               /* For file ops */
#include <linux/module.h>           /* For module macros */
#include <linux/uaccess.h>          /* Copy to/from user */

#include "exchardev.h"

static const char * const green = "\033[0;32m";
static const char * const red = "\033[0;31m";
static const char * const defcol = "\033[0m";

static struct class *exchardev_class;
static struct device *exchardev;
static dev_t dev;
static dev_t dev_region;
static color_t color;

/* Character device file operations callbacks */
static int exchardev_open(struct inode *inode, struct file *file);
static int exchardev_release(struct inode *inode, struct file *file);
static ssize_t exchardev_read(struct file *file, char *buf, size_t size,
                              loff_t *offset);
static ssize_t exchardev_write(struct file *file, const char *buf, size_t size,
                               loff_t *offset);
static long exchardev_ioctl(struct file *file, unsigned int cmd,
                           unsigned long arg);

static struct file_operations fops = {
    .open = exchardev_open,
    .release = exchardev_release,
    .read = exchardev_read,
    .write = exchardev_write,
    .unlocked_ioctl = exchardev_ioctl,
};

static const char *get_color(int color)
{
    switch (color) {
    case COLOR_DEF:
        return defcol;
    case COLOR_RED:
        return red;
    case COLOR_GREEN:
        return green;
    default:
        return NULL;
    }
}

static int exchardev_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO EXCHARDEV ": Device opened\n");
    return 0;
}

static int exchardev_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO EXCHARDEV ": Device closed\n");
    return 0;
}

static ssize_t exchardev_read(struct file *file, char *buf, size_t size,
                              loff_t *offset)
{
    const char *msg = "Hello from kernel";
    size_t bytes_to_copy;
    int err;

    /* Handle data size */
    bytes_to_copy = strlen(msg);
    if (bytes_to_copy > size) {
        bytes_to_copy = size;
    }

    /* Copy data to user */
    err = copy_to_user(buf, &msg[*offset], bytes_to_copy);
    if (err) {
        return -EFAULT;
    }

    return bytes_to_copy;
}

static ssize_t exchardev_write(struct file *file, const char *buf, size_t size,
                               loff_t *offset)
{
    char msg[100] = {0};
    size_t bytes_to_copy;
    int err;

    /* Handle data size */
    bytes_to_copy = sizeof(msg);
    if (size < bytes_to_copy) {
        bytes_to_copy = size;
    }

    /* Copy data from user */
    err = copy_from_user(&msg[*offset], buf, bytes_to_copy);
    if (err) {
        return -EFAULT;
    }

    printk(KERN_INFO EXCHARDEV ": Kernel received the next message: %s%s%s\n",
           get_color(color), msg, defcol);

    return bytes_to_copy;
}

static long exchardev_ioctl(struct file *file, unsigned int cmd,
                           unsigned long arg)
{
    int err;

    switch (cmd) {
    case EXCHARDEV_SET_COLOR:
        if ((color_t)arg >= LAST_COLOR) {
            return -EFAULT;
        }
        color = (color_t)arg;
        printk(KERN_INFO EXCHARDEV ": Color set to %d\n", color);
        break;
    case EXCHARDEV_GET_COLOR:
        err = copy_to_user((color_t *)arg, &color, sizeof(color));
        if (err) {
            return -EFAULT;
        }
        printk(KERN_INFO EXCHARDEV ": Current color is %d\n", color);
        break;
    default:
        return -EFAULT;
    }

    return 0;
}

static int exchardev_init(void)
{
    int ret;

    /* Make devices based on major and minor numbers */
    dev = MKDEV(EXCHARDEV_MAJOR, 0);
    dev_region = MKDEV(EXCHARDEV_MAJOR, 1);

    /* Register character device */
    ret = register_chrdev(MAJOR(dev), EXCHARDEV, &fops);
    if (ret < 0) {
        return ret;
    }

    /* Create class */
    exchardev_class = class_create(THIS_MODULE, EXCHARDEV_CLASS);
    if (!exchardev_class) {
        unregister_chrdev(MAJOR(dev), EXCHARDEV);
        return -EFAULT;
    }

    /* Create character device */
    exchardev = device_create(exchardev_class, NULL, dev, NULL, EXCHARDEV"%d",
                              MINOR(dev));
    if (!exchardev) {
        class_destroy(exchardev_class);          
        unregister_chrdev(MAJOR(dev), EXCHARDEV);
        return -EFAULT;
    }

    printk(KERN_INFO EXCHARDEV ": Example character device is inserted\n");

    return 0;
}

static void exchardev_exit(void)
{
    device_destroy(exchardev_class, dev);
    class_unregister(exchardev_class);
    class_destroy(exchardev_class);
    unregister_chrdev(MAJOR(dev), EXCHARDEV);

    printk(KERN_INFO EXCHARDEV ": Example character device is removed\n");
}

MODULE_LICENSE("GPL");
module_init(exchardev_init);
module_exit(exchardev_exit);
