#ifndef SSA_H
#define SSA_H

#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/types.h>

struct ssa_dev {
    char **data; /* Array to pointers of data, each of size l2_sze */

    size_t l1_sze; /* Size of pointer pool */
    size_t l2_sze; /* Size of each block of data */

    struct cdev cdev;
    struct class *class;
};
#ifndef D_NAME
#define D_NAME(STRUCT_FILE) ((STRUCT_FILE)->f_path.dentry->d_name.name)
#endif

/* Prototypes */
/* static int __init ssa_load(void);
static void __exit ssa_unload(void);
static int ssa_open(struct inode *inode, struct file *file);
static int ssa_release(struct inode *inode, struct file *file);
static ssize_t ssa_read(struct file *filp, char __user *buf, size_t len,
                        loff_t *off);
static ssize_t ssa_write(struct file *filp, const char *buf, size_t len,
                         loff_t *off); */
#ifndef SSA_L2_SZE
#define SSA_L2_SZE 1024
#endif

#ifndef SSA_L1_SZE
#define SSA_L1_SZE 8
#endif

#endif /* SSA_H */
