#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>

#define ETX_NUM_DEVS 2

#define D_NAME(STRUCT_FILE) ((STRUCT_FILE)->f_path.dentry->d_name.name)

int etx_nr_devs = ETX_NUM_DEVS;
int etx_dev_major;
int etx_dev_minor;

dev_t etx_dev = 0;
static struct class *etx_class;
static struct cdev *etx_cdevs;

/*
** Function Prototypes
*/
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len,
                        loff_t *off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len,
                         loff_t *off);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = etx_read,
    .write = etx_write,
    .open = etx_open,
    .release = etx_release,
};

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *filep) {
    /** pr_info("etx_open called on %s\n", file->f_path.dentry->d_name.name); */
    pr_info("etx_open called on %s\n", D_NAME(filep));
    return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *filep) {
    pr_info("etx_release called on %s\n", D_NAME(filep));
    return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filep, char __user *buf, size_t len,
                        loff_t *off) {
    pr_info("etx_read called on %s\n", D_NAME(filep));
    return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filep, const char __user *buf, size_t len,
                         loff_t *off) {
    pr_info("etx_write called on %s\n", D_NAME(filep));
    return len;
}

/** function to create cdev struct */
static int etx_setup_cdev(struct cdev *cdev, int index) {
    int err, dev_num = MKDEV(etx_dev_major, etx_dev_minor + index);
    cdev_init(cdev, &fops);
    cdev->owner = THIS_MODULE;
    err = cdev_add(cdev, dev_num, 1);
    if (err) {
        pr_info("Error in adding etx%d\n", index);
        return err;
    }
    pr_info("Loaded driver number %d\n", index);
    return 0;
}

/*
 * Callback when device file is created, change permission to 0666
 */
static char *etx_devnode(struct device *dev, umode_t *mode) {
    if (!mode)
        return NULL;
    *mode = 0666;
    return NULL;
}

/** Create class */
static int __init etx_class_init(void) {
    etx_class = class_create(THIS_MODULE, "etx");
    if (IS_ERR(etx_class))
        return PTR_ERR(etx_class);
    etx_class->devnode = etx_devnode;
    return 0;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void) {
    int result, i;
    /*Allocating Major number*/
    if ((alloc_chrdev_region(&etx_dev, 0, 2, "supprocfs")) < 0) {
        pr_err("Cannot allocate device\n");
        return -1;
    }
    etx_dev_major = MAJOR(etx_dev);
    etx_dev_minor = MINOR(etx_dev);
    pr_info("etx added\n");

    etx_cdevs = kmalloc(etx_nr_devs * sizeof(struct cdev), GFP_KERNEL);
    if (!etx_cdevs) {
        result = -ENOMEM;
        goto fail;
    }
    memset(etx_cdevs, 0, etx_nr_devs * sizeof(struct cdev));

    etx_class_init();
    for (i = 0; i < etx_nr_devs; i++) {
        etx_setup_cdev(etx_cdevs, i);
        device_create(etx_class, NULL, MKDEV(etx_dev_major, etx_dev_minor + i),
                      NULL, "koolchardrv%d", i);
    }
    return 0;
fail:
    return result;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void) {
    int i;
    for (i = 0; i < etx_nr_devs; i++) {
        /** cdev_del(&etx_cdevs[i]); */
        device_destroy(etx_class, MKDEV(etx_dev_major, etx_dev_minor + i));
    }
    kfree(etx_cdevs);
    class_unregister(etx_class);
    class_destroy(etx_class);
    unregister_chrdev_region(etx_dev, etx_nr_devs);
    pr_info("Removed etx\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("God");
MODULE_DESCRIPTION("test driver");
