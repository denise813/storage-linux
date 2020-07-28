#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/cdev.h>

#define DEMO_NAME "c_demo_dev"

#define dprintk printk

static dev_t dev;
static struct cdev * demo_cdev = NULL;
static signed count = 1;

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

static int simple_char_init(void)
{
	int ret = 0;

	ret = alloc_chrdev_region(&dev, 0, count, DEMO_NAME);
	if (ret != 0)
	{
		dprintk("failed to alloc_chardev_region dev\n");
		goto l_out;
	}

	demo_cdev = cdev_alloc();
	if (demo_cdev == NULL)
	{
		dprintk("failed to cdev_alloc dev\n");
		goto l_out;
	}

	cdev_init(demo_cdev, &demodrv_fops);

	ret = cdev_add(demo_cdev, dev, count);
	if (ret!= 0)
	{
		dprintk("failed to cdev_add dev\n");
		goto l_out;
	}

	dprintk("succeeded init char device (%s), Major(%d), Minor(%d)\n", DEMO_NAME, MAJOR(dev), MINOR(dev));


l_out:
	return ret;
}

static void __exit simple_char_exit(void)
{
	if (demo_cdev != NULL)
		cdev_del(demo_cdev);

	dprintk("succeeded exit char device (%s), Major(%d), Minor(%d)\n", DEMO_NAME, MAJOR(dev), MINOR(dev));

	unregister_chrdev_region(dev, count);

	return;
}

module_init(simple_char_init);
module_exit(simple_char_exit);

