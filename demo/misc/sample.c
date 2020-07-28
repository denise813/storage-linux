#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/miscdevice.h>

#define DEMO_NAME "misc_demo_dev"

#define dprintk printk

static int demodrv_open(struct inode * p_inode, struct file * p_file)
{
	int major = MAJOR(p_inode->i_rdev);
	int minor = MINOR(p_inode->i_rdev);

	dprintk("%s: major=%d minor=%d\n", __func__, major, minor);
	return 0;
}

static int demodrv_release(struct inode * p_inode, struct file * p_file)
{
	dprintk("%s\n", __func__);
	return 0;
}

static ssize_t demodrv_read(struct file * p_file, char __user * p_buf, size_t count, loff_t * p_pos)
{
	dprintk("%s\n", __func__);
	return 0;
}

static ssize_t demodev_write(struct file * p_file, const char __user * p_buf, size_t count, loff_t * p_pos)
{
	dprintk("%s\n", __func__);
	return 0;
}

static const struct file_operations demodrv_fops = {
	.owner = THIS_MODULE,
	.open = demodrv_open,
	.release = demodrv_release,
	.read = demodrv_read,
	.write = demodev_write,
};

#define MISC_DYNAMIC_MINOR 433
static struct device * dev = NULL;
static struct miscdevice misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEMO_NAME,
	.fops = &demodrv_fops,
};


static int simple_misc_init(void)
{
	int ret = 0;

	ret = misc_register(&misc_device);
	if (ret != 0)
	{
		dprintk("failed to alloc_chardev_region dev\n");
		goto l_out;
	}

	dev = misc_device.this_device;

	dprintk("succeeded init misc device (%s)\n", DEMO_NAME);


l_out:
	return ret;
}

static void __exit simple_misc_exit(void)
{

	dprintk("succeeded exit misc device (%s)\n", DEMO_NAME);

	misc_deregister(&misc_device);

	return;
}

module_init(simple_misc_init);
module_exit(simple_misc_exit);

