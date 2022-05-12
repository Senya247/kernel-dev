#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define SSA_NUM_DEVS (2)
#define SSA_BUF_SZE (6)

#define D_NAME(STRUCT_FILE) ((STRUCT_FILE)->f_path.dentry->d_name.name)

int ssa_nr_devs = SSA_NUM_DEVS;
int ssa_dev_major;
int ssa_dev_minor;

char ssa_buf[SSA_BUF_SZE] = {'h', 'e', 'l', 'l', 'o', 'e'};
char *ssa_buf_end = ssa_buf + SSA_BUF_SZE - 1;

dev_t ssa_dev = 0;
static struct class *ssa_class;
static struct cdev *ssa_cdevs;

/*
** Function Prototypes
*/
static int __init ssa_load(void);
static void __exit ssa_unload(void);
static int ssa_open(struct inode *inode, struct file *file);
static int ssa_release(struct inode *inode, struct file *file);
static ssize_t ssa_read(struct file *filp, char __user *buf, size_t len,
                        loff_t *off);
static ssize_t ssa_write(struct file *filp, const char *buf, size_t len,
                         loff_t *off);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = ssa_read,
    .write = ssa_write,
    .open = ssa_open,
    .release = ssa_release,
};

/*
** This function will be called when we open the Device file
*/
static int ssa_open(struct inode *inode, struct file *filep) {
    /** pr_info("ssa_open called on %s\n", file->f_path.dentry->d_name.name); */
    pr_info("ssa_open called on %s\n", D_NAME(filep));
    return 0;
}

/*
** This function will be called when we close the Device file
*/
static int ssa_release(struct inode *inode, struct file *filep) {
    pr_info("ssa_release called on %s\n", D_NAME(filep));
    return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t ssa_read(struct file *filep, char __user *buf, size_t len,
                        loff_t *off) {
    pr_info("ssa_read called on %s\n", D_NAME(filep));
    ssize_t retval = 0;
    unsigned long to_write;
    off_t buf_off = *off;

    while (len) {
        if (buf_off >= SSA_BUF_SZE)
            buf_off = buf_off % SSA_BUF_SZE;

        to_write =
            (SSA_BUF_SZE - buf_off) < len ? (SSA_BUF_SZE - buf_off) : len;
        if (copy_to_user(buf + *off, ssa_buf + buf_off, to_write)) {
            retval = -EFAULT;
            goto out;
        }

        len -= to_write;
        retval += to_write;
        *off += to_write;
        buf_off += to_write;
        /** pr_info("len: %zu, off: %lld, to_write: %lu\n", len, *off,
         * to_write); */
    }
out:
    return retval;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t ssa_write(struct file *filep, const char __user *buf, size_t len,
                         loff_t *off) {
    pr_info("ssa_write called on %s\n", D_NAME(filep));
    ssize_t retval = 0;
    unsigned long to_read;
    off_t buf_off = *off;

    while (len) {
        if (buf_off >= SSA_BUF_SZE)
            buf_off = buf_off % SSA_BUF_SZE;

        to_read = (SSA_BUF_SZE - buf_off) < len ? (SSA_BUF_SZE - buf_off) : len;
        if (copy_from_user(ssa_buf + buf_off, buf + *off, to_read)) {
            retval = -EFAULT;
            goto out;
        }

        len -= to_read;
        retval += to_read;
        *off += to_read;
        buf_off += to_read;
        /** pr_info("len: %zu, off: %lld, to_read: %lu\n", len, *off,
         * to_read); */
    }
out:
    return retval;
}

/** function to create cdev struct */
static int ssa_setup_cdev(struct cdev *cdev, int index) {
    int err, dev_num = MKDEV(ssa_dev_major, ssa_dev_minor + index);
    cdev_init(cdev, &fops);
    cdev->owner = THIS_MODULE;
    err = cdev_add(cdev, dev_num, 1);
    if (err) {
        pr_info("Error in adding ssa%d\n", index);
        return err;
    }
    pr_info("Loaded driver number %d\n", index);
    return 0;
}

/*
 * Callback when device file is created, change permission to 0666
 */
static char *ssa_devnode(struct device *dev, umode_t *mode) {
    if (!mode)
        return NULL;
    *mode = 0666;
    return NULL;
}

/** Create class */
static int __init ssa_class_init(void) {
    ssa_class = class_create(THIS_MODULE, "ssa");
    if (IS_ERR(ssa_class))
        return PTR_ERR(ssa_class);
    ssa_class->devnode = ssa_devnode;
    return 0;
}

/*
** Module Init function
*/
static int __init ssa_load(void) {
    int result, i;
    /*Allocating Major number*/
    if ((alloc_chrdev_region(&ssa_dev, 0, 2, "supprocfs")) < 0) {
        pr_err("Cannot allocate device\n");
        return -1;
    }
    ssa_dev_major = MAJOR(ssa_dev);
    ssa_dev_minor = MINOR(ssa_dev);
    pr_info("ssa added\n");

    ssa_cdevs = kmalloc(ssa_nr_devs * sizeof(struct cdev), GFP_KERNEL);
    if (!ssa_cdevs) {
        result = -ENOMEM;
        goto fail;
    }
    memset(ssa_cdevs, 0, ssa_nr_devs * sizeof(struct cdev));

    ssa_class_init();
    for (i = 0; i < ssa_nr_devs; i++) {
        ssa_setup_cdev(ssa_cdevs, i);
        device_create(ssa_class, NULL, MKDEV(ssa_dev_major, ssa_dev_minor + i),
                      NULL, "koolchardrv%d", i);
    }
    /** memset(ssa_buf, (int)'x', SSA_BUF_SZE); */

    return 0;
fail:
    return result;
}

/*
** Module exit function
*/
static void __exit ssa_unload(void) {
    int i;
    for (i = 0; i < ssa_nr_devs; i++) {
        /** cdev_del(&ssa_cdevs[i]); */
        device_destroy(ssa_class, MKDEV(ssa_dev_major, ssa_dev_minor + i));
    }
    kfree(ssa_cdevs);
    class_unregister(ssa_class);
    class_destroy(ssa_class);
    unregister_chrdev_region(ssa_dev, ssa_nr_devs);
    pr_info("Unloaded ssa\n");
}

module_init(ssa_load);
module_exit(ssa_unload);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("God");
MODULE_DESCRIPTION("test driver");
