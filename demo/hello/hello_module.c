#include <linux/init.h>
#include <linux/module.h>

static int debug = 1;

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "enable debuging information");

#define dprintk(args, ...) \
do{	\
	if (debug) {	\
		printk(KERN_DEBUG args);	\
	}	\
}	\
while(0);

static int __init my_hello_init(void)
{
	printk("debug=%d", debug);
	dprintk("hello moudle init\n");
	return 0;
}

static void __exit my_hello_exit(void)
{
	dprintk("hello moudle exit\n");
	return;
}

module_init(my_hello_init);
module_exit(my_hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Denise");
MODULE_DESCRIPTION("my hello demo kernel moudle");
MODULE_ALIAS("my hello");
