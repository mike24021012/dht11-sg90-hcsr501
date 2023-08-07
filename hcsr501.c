#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <asm-generic/uaccess.h>
#include <linux/spinlock.h>

#define DEVICE_MAJOR 240
#define DEVICE_NAME "hcsr501"
#define PIN	EXYNOS4_GPX0(6)

unsigned char buf[1]={'0'};

static ssize_t infrared_read(struct file *file, char* buffer, size_t size, loff_t *off)
{
   int ret;
       
   if(gpio_get_value(PIN))
	{
		buf[0]='1';
		ret = copy_to_user(buffer,buf,sizeof(buf));
		if(ret < 0)
		{
			printk("copy to user err\n");
			return -EAGAIN;
        }
        else
        {
            return 0;
		}
	}
	else
	{
		buf[0]='0';
		ret = copy_to_user(buffer,buf,sizeof(buf));
		if(ret < 0)
		{
			printk("copy to user err\n");
			return -EAGAIN;
        }
        else
        {
            return 0;
		}
	}
	
	return 0;
}

static int infrared_open(struct inode *inode, struct file *file)
{
    printk("open in kernel\n");
	
	int ret;
	// Reserve gpios
	if( gpio_request( PIN, DEVICE_NAME ) < 0 )	// request gpx0(6)
	{
		printk( KERN_INFO "%s: %s unable to get gpio\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}
	printk("hcsr501 gpio request ok!\n");
	// Set gpios directions
	if( gpio_direction_input(PIN) < 0 )	// Set gpx0(6) as input 
	{
		printk( KERN_INFO "%s: %s unable to set gpio as input\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}
	printk("hcsr501 gpio setup ok!\n");
    
    return 0;
}

static int infrared_release(struct inode *inode, struct file *file)
{
	gpio_free(PIN);
    printk("hcsr501 gpio release\n");
    return 0;
}

static struct file_operations infrared_dev_fops={
    owner		:	THIS_MODULE,
    open		:	infrared_open,
	read		:	infrared_read,
	release		:	infrared_release,
};
	
static struct class *hcsr501_class;

static int __init infrared_dev_init(void) 
{
	int	ret;

	ret = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &infrared_dev_fops);
	if (ret < 0) {
		printk(KERN_INFO "%s: registering device %s with major %d failed with %d\n",
		       __func__, DEVICE_NAME, DEVICE_MAJOR, DEVICE_MAJOR );
		return(ret);
	}
	printk("hcsr501 driver register success!\n");
	
	hcsr501_class = class_create(THIS_MODULE, "hcsr501");
    if (IS_ERR(hcsr501_class))
	{
		printk(KERN_WARNING "Can't make node %d\n", DEVICE_MAJOR);
                return PTR_ERR(hcsr501_class);
	}

    device_create(hcsr501_class, NULL, MKDEV(DEVICE_MAJOR, 0), NULL, DEVICE_NAME);
        
	printk("hcsr501 driver make node success!\n");
	
    return 0;
}

static void __exit infrared_dev_exit(void)
{
	printk("exit in kernel\n");
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
	printk("Remove hcsr501 device success!\n");
}

module_init(infrared_dev_init);
module_exit(infrared_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WWW");
