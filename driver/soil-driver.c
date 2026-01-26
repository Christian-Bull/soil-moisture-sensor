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
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include "soil-driver.h"

#define ADS1115_CONVERSION_REG   0x00
#define ADS1115_CONFIG_REG       0x01
#define ADS1115_CONFIG_SINGLE_A0_4096_128SPS  0xC383

#define I2C_BUS_NUM   1
#define ADS1115_ADDR  0x48

static struct i2c_adapter *soil_adap;

int soil_major =   0; // use dynamic major
int soil_minor =   0;
static struct class *soil_class;
static struct device *soil_device_ptr;

MODULE_AUTHOR("Christian Bull");
MODULE_LICENSE("Dual BSD/GPL");

struct soil_dev soil_device;

static int ads1115_start_single_conversion(struct i2c_client *client, u16 cfg)
{
	int ret;

	ret = i2c_smbus_write_word_data(client, ADS1115_CONFIG_REG, swab16(cfg));
	return ret;
}

static int ads1115_read_raw(struct i2c_client *client, s16 *out_raw)
{
	int ret;
	u16 word;

	ret = i2c_smbus_read_word_data(client, ADS1115_CONVERSION_REG);
	if (ret < 0)
		return ret;

	word = (u16)ret;
	word = swab16(word);
	*out_raw = (s16)word;

	return 0;
}

int soil_open(struct inode *inode, struct file *filp)
{
    struct soil_dev *dev;

    PDEBUG("open");

    dev = container_of(inode->i_cdev, struct soil_dev, cdev);
    filp->private_data = dev;

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
	struct soil_dev *dev = filp->private_data;
	s16 raw;
	int ret;
	double volts;
	char kbuf[64];
	int len;

	if (*f_pos > 0)
		return 0;

	if (!dev || !dev->client)
		return -ENODEV;

	ret = ads1115_start_single_conversion(dev->client,
			ADS1115_CONFIG_SINGLE_A0_4096_128SPS);
	if (ret < 0)
		return ret;

	// Wait for conversion (~8 ms @128 SPS)
	usleep_range(9000, 12000);

	// Read conversion result (2 bytes)
	ret = ads1115_read_raw(dev->client, &raw);
	if (ret < 0)
		return ret;

	volts = (double)raw * 4.096 / 32768.0;

	// format output for userspace
	len = scnprintf(kbuf, sizeof(kbuf), "Raw: %d\tVoltage: %.4f V\n", raw, volts);

	if (count < len)
		len = count;

	if (copy_to_user(buf, kbuf, len))
		return -EFAULT;

	*f_pos += len;
	return len;
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


    soil_adap = i2c_get_adapter(I2C_BUS_NUM);
    if (!soil_adap) {
        pr_err("soil-driver: failed to get i2c adapter %d\n", I2C_BUS_NUM);
        result = -ENODEV;
        goto err_unregister_chrdev;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
    soil_device.client = i2c_new_dummy_device(soil_adap, ADS1115_ADDR);
#else
    soil_device.client = i2c_new_dummy(soil_adap, ADS1115_ADDR);
#endif
    if (IS_ERR(soil_device.client)) {
        pr_err("soil-driver: failed to create i2c client at 0x%02x\n", ADS1115_ADDR);
        result = PTR_ERR(soil_device.client);
        soil_device.client = NULL;
        goto err_put_adapter;
    }

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

err_destroy_device:
    device_destroy(soil_class, dev);
err_destroy_class:
    class_destroy(soil_class);
    soil_class = NULL;
err_del_cdev:
    cdev_del(&soil_device.cdev);
err_unregister_chrdev:
    unregister_chrdev_region(dev, 1);
    return result;
}
 
void soil_cleanup_module(void)
{
    dev_t devno = MKDEV(soil_major, soil_minor);

    cdev_del(&soil_device.cdev);

    unregister_chrdev_region(devno, 1);
    
    device_destroy(soil_class, devno);
    
    class_destroy(soil_class);

    if (soil_device.client) {
    i2c_unregister_device(soil_device.client);
    soil_device.client = NULL;
    }

    if (soil_adap) {
        i2c_put_adapter(soil_adap);
        soil_adap = NULL;
    }
}

module_init(soil_init_module);
module_exit(soil_cleanup_module);
