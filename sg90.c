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

#define DEVICE_MAJOR 248
#define DEVICE_NAME "sg90"
#define PIN	EXYNOS4_GPD0(3)
#define MAGIC_NUMBER 'X'
#define PWM_IOCTL_SET_FREQ	_IO(MAGIC_NUMBER,  1)

static long sg90_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{

	//positive rotation, speed level: 0~9  (pulse duration time: 14ms~5ms)
	//negative rotation, speed level: 0~9  (pulse duration time: 16ms~25ms)
	//stop (pulse duration time: 15ms)
	
	int i,j;
	
	switch (cmd) 
	{
	case PWM_IOCTL_SET_FREQ:
		for(i=0;i<20;++i)
		{
			if( gpio_direction_output(PIN,1) < 0 )
			{
				printk("set gpio output value 1 fail.\n");
				return -1;
			}
			for(j=0;j<arg;++j)
				udelay(100);
		
			if( gpio_direction_output(PIN,0) < 0 )
    		{
    			printk("set gpio output value 0 fail.\n");
				return -1;
    		}
       		for(j=0;j<(200-arg);++j)
        		udelay(100);     
		}
		break;
	default:
		break;
	}
	
	return 0;
}

static int sg90_open(struct inode *inode, struct file *file)
{
    printk("open in kernel\n");
	
	int ret;
	// Reserve gpios
	if( gpio_request( PIN, DEVICE_NAME ) < 0 )	// request gpx0(6)
	{
		printk("%s unable to get a gpio\n", DEVICE_NAME);
		ret = -EBUSY;
		return(ret);
	}
	printk("sg90 gpio request ok!\n");
	// Set gpios directions
	if( gpio_direction_output(PIN,0) < 0 )	// Set gpx0(6) as output and output 0 
	{
		printk("%s unable to set gpio as output\n", DEVICE_NAME);
		ret = -EBUSY;
		return(ret);
	}
    printk("sg90 gpio setup ok!\n");
	
    return 0;
}

static int sg90_release(struct inode *inode, struct file *file)
{
	gpio_free(PIN);
    printk("sg90 gpio release\n");
    return 0;
}

static struct file_operations sg90_dev_fops={
    owner			:	THIS_MODULE,
    open			:	sg90_open,
    unlocked_ioctl  :   sg90_ioctl,
	release			:	sg90_release,
};
	
static struct class *sg90_class;

static int __init sg90_dev_init(void) 
{
	int	ret;

	ret = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &sg90_dev_fops);
	if (ret < 0) {
		printk("fail in registering device %s with major %d failed with %d\n",
		        DEVICE_NAME, DEVICE_MAJOR, DEVICE_MAJOR );
		return(ret);
	}
	printk("sg90 driver register success!\n");
	
	sg90_class = class_create(THIS_MODULE, "sg90");
    if (IS_ERR(sg90_class))
	{
		printk("Can't make node %d\n", DEVICE_MAJOR);
        return PTR_ERR(sg90_class);
	}

    device_create(sg90_class, NULL, MKDEV(DEVICE_MAJOR, 0), NULL, DEVICE_NAME);    
	printk("sg90 driver make node success!\n");
	
    return 0;
}

static void __exit sg90_dev_exit(void)
{
	printk("exit in kernel\n");
    unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
	printk("Remove sg90 device success!\n");
}

module_init(sg90_dev_init);
module_exit(sg90_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WWW");
