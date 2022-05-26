#include "ssa.h"
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

dev_t ssa_dev;
int ssa_major;
int ssa_minor;
int ssa_nr_devs = 3;

struct class *ssa_class;

MODULE_LICENSE("GPL");
MODULE_AUTHOR("God");
MODULE_DESCRIPTION("yeet");

struct ssa_dev *ssa_devices;

/*
** This function will be called when we open the Device file
*/
static int ssa_open(struct inode *inode, struct file *filep) {
    /** pr_info("ssa_open called on %s\n", file->f_path.dentry->d_name.name); */
    pr_info("ssa_open called on %s\n", D_NAME(filep));
    struct ssa_dev *s_dev;

    /* To identify later on which device(ssa0, ssa1 etc) is being used */
    s_dev = container_of(inode->i_cdev, struct ssa_dev, cdev);
    filep->private_data = s_dev;

    return 0;
}

/*
** This function will be called when we close the Device file
*/
static int ssa_release(struct inode *inode, struct file *filep) {
    pr_info("ssa_release called on %s\n\n", D_NAME(filep));

    filep->private_data = NULL;

    return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t ssa_read(struct file *filep, char __user *buf, size_t len,
                        loff_t *off) {}

/*
** This function will be called when we write the Device file
*/
static ssize_t ssa_write(struct file *filep, const char __user *buf, size_t len,
                         loff_t *off) {
    pr_info("ssa_write called on %s\n", D_NAME(filep));
    ssize_t ret = 0;
    struct ssa_dev *s_dev;
    char *buffer;

    s_dev = filep->private_data;
    if (*off >= s_dev->l1_sze * s_dev->l2_sze) {
        goto fin;
    }

    buffer = kmalloc(len, GFP_KERNEL);
    if (!buffer) {
        ret = -ENOMEM;
        goto fin;
    }
    if (copy_from_user(buffer, buf, len)) {
        ret = -EFAULT;
        goto fin;
    }

    size_t l2_off, to_copy, cur_index;

    cur_index = *off / s_dev->l2_sze;
    char *cur = s_dev->data[cur_index];

    /* Loop until either all the data is copied or no space is left */
    while ((ret < len) && (cur_index != s_dev->l1_sze)) {
        if (cur == NULL)
            cur = kmalloc(s_dev->l2_sze, GFP_KERNEL);
        l2_off = *off % s_dev->l2_sze;
        to_copy = s_dev->l2_sze - l2_off;
        to_copy = to_copy < (len - ret) ? to_copy : (len - ret);

        memcpy(cur + l2_off, buffer + ret, to_copy);
        pr_info("Copied %*.*s, to_copy: %ld\n", to_copy, to_copy, cur + l2_off,
                to_copy);

        *off += to_copy;
        ret += to_copy;
        cur_index++;
    }

fin:
    pr_info("ret: %ld\n", ret);
    kfree(buffer);
    return ret;
}

/** File operations */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = ssa_read,
    .write = ssa_write,
    .open = ssa_open,
    .release = ssa_release,
};

/** Function to create cdev struct */
static int ssa_setup_cdev(struct cdev *cdev, int index) {
    int err, dev_num = MKDEV(ssa_major, ssa_minor + index);

    pr_info("Initing cdev\n");
    cdev_init(cdev, &fops);
    cdev->owner = THIS_MODULE;

    pr_info("adding cdev");
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

/** Initialize ssa_devices */
static int ssa_devices_init(void) {
    int i, j;
    struct ssa_dev *s_dev;
    for (i = 0; i < ssa_nr_devs; i++) {
        pr_info("entered loop\n");
        s_dev = &ssa_devices[i];

        s_dev->class = ssa_class;
        pr_info("Creating cdev\n");
        ssa_setup_cdev(&s_dev->cdev, i);
        pr_info("Creating dev %d\n", i);
        device_create(ssa_class, NULL, MKDEV(ssa_major, ssa_minor + i), NULL,
                      "ssa%d", i);

        pr_info("setting buffer sizes and mallocing l1\n");
        s_dev->index = i;
        s_dev->l1_sze = SSA_L1_SZE;
        s_dev->l2_sze = SSA_L2_SZE;
        s_dev->data = kmalloc(s_dev->l1_sze * sizeof(char *),
                              GFP_KERNEL); /** L2 is malloced only when it is
                                              needed by a userspace program */

        /* Data was memsetted to 0 */
        if (!s_dev->data)
            return -ENOMEM;

        pr_info("Entering deadly for loop\n");
        for (j = 0; j < s_dev->l1_sze; j++)
            s_dev->data[j] = NULL;
    }
    return 0;
}

/*
** Module Init function
*/
static int __init ssa_load(void) {
    int result, t;
    /*Allocating Major number*/
    pr_info("Allocing region\n");
    if ((alloc_chrdev_region(&ssa_dev, 0, ssa_nr_devs, "ssa")) < 0) {
        pr_err("Cannot allocate device\n");
        return -1;
    }

    ssa_major = MAJOR(ssa_dev);
    ssa_minor = MINOR(ssa_dev);

    pr_info("mallocing ssa_devices\n");
    ssa_devices = kmalloc(ssa_nr_devs * sizeof(struct ssa_dev), GFP_KERNEL);
    if (!ssa_devices) {
        result = -ENOMEM;
        goto fail;
    }
    pr_info("Memsetting ssa_devices\n");
    memset(ssa_devices, 0, ssa_nr_devs * sizeof(struct ssa_dev));

    pr_info("Initializing class\n");
    if ((t = ssa_class_init())) {
        result = t;
        goto fail;
    }
    pr_info("Initializing ssa_devices\n");
    if ((t = ssa_devices_init())) {
        result = t;
        goto fail;
    }

    pr_info("Loaded ssa\n");
    return 0;

fail:
    return result;
}

static void __exit ssa_unload(void) {
    int i, j;
    struct ssa_dev *s_dev;
    for (i = 0; i < ssa_nr_devs; i++) {
        s_dev = &ssa_devices[i];
        for (j = 0; j < s_dev->l1_sze; j++) {
            /* if one is not malloced yet, all the others after aren't malloced
             * either */
            if (!s_dev->data[j])
                continue;
            kfree(s_dev->data[j]);
        }
        kfree(s_dev->data);

        device_destroy(ssa_class, MKDEV(ssa_major, ssa_minor + i));

        cdev_del(&s_dev->cdev);
    }
    pr_info("unregistering class\n");
    class_unregister(ssa_class);

    pr_info("destroying class\n");
    class_destroy(ssa_class);
    unregister_chrdev_region(ssa_dev, ssa_nr_devs);
    pr_info("Unloaded ssa\n");
}

module_init(ssa_load);
module_exit(ssa_unload);
