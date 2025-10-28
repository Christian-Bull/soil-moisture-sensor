/**
 * @file soil-driver.c
 * @author Christian Bull
 * @date 2025-10-23
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/version.h>
#include "soil-driver.h"

int soil_major =   0; // use dynamic major
int soil_minor =   0;
static struct class *soil_class;
static struct device *soil_device_ptr;

MODULE_AUTHOR("Christian Bull");
MODULE_LICENSE("Dual BSD/GPL");

struct soil_dev soil_device;

int soil_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");

    return 0;
}

int soil_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");

    return 0;
}

ssize_t soil_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);

    return retval;
}

ssize_t soil_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);

    return retval;
}
struct file_operations soil_fops = {
    .owner =    THIS_MODULE,
    .read =     soil_read,
    .write =    soil_write,
    .open =     soil_open,
    .release =  soil_release,
};

static int soil_setup_cdev(struct soil_dev *dev)
{
    int err, devno = MKDEV(soil_major, soil_minor);

    cdev_init(&dev->cdev, &soil_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &soil_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding soil cdev", err);
    }
    return err;
}



int soil_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, soil_minor, 1, "soil-driver");

    soil_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", soil_major);
        return result;
    }
    memset(&soil_device,0,sizeof(struct soil_dev));

    result = soil_setup_cdev(&soil_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }

    printk(KERN_INFO "soil_driver: registered with major %d\n", soil_major);

#ifdef class_create
    /* Macro form → takes 2 arguments */
    soil_class = class_create(THIS_MODULE, "soil-driver");
#else
    /* Function form → takes 1 argument */
    soil_class = class_create("soil-driver");
#endif

    if (IS_ERR(soil_class)) {
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(soil_class);
    }

    soil_device_ptr = device_create(soil_class, NULL, dev, NULL, "soil-driver");
    if (IS_ERR(soil_device_ptr)) {
        class_destroy(soil_class);
        unregister_chrdev_region(dev, 1);
        return PTR_ERR(soil_device_ptr);
    }

    return result;

}

void soil_cleanup_module(void)
{
    dev_t devno = MKDEV(soil_major, soil_minor);

    cdev_del(&soil_device.cdev);

    unregister_chrdev_region(devno, 1);
    
    device_destroy(soil_class, devno);
    
    class_destroy(soil_class);
}

module_init(soil_init_module);
module_exit(soil_cleanup_module);
