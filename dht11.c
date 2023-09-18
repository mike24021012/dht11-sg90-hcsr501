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

#define DEVICE_MAJOR 232
#define DEVICE_NAME "dht11"
#define PIN	EXYNOS4_GPX0(2)

unsigned char buf[5];
unsigned char check_flag,check_parity;

void humidity_read_data(void)
{
	int i,j,num; 
    	unsigned char flag;
	unsigned char data;
	//host transmit start signal to dht11, and when dht11 receive the signal, it will transmit echo signal to host.
	gpio_direction_output(PIN,0);
	mdelay(30);
	gpio_direction_output(PIN,1);
	udelay(40);
	//host change gpio to input mode, and will receive 80us low level signal and then 80us high level signal.
	gpio_direction_input(PIN);
	if(gpio_get_value(PIN) == 0)
	{ 
		i=0;
		while(!gpio_get_value(PIN))
		{
			udelay(5);
			i++;
			if(i>20)
			{
				printk("error data!\n");
				break;
        		}
		}
		i=0;
		while(gpio_get_value(PIN))
		{
        		udelay(5);
        		i++;
        		if(i>20)
        		{
				printk("error data!\n");
				break;
			}
		}
	}
	//start to input data, total 40 bits, 16 bits for temperature, 16 bits for humidity, 8 bits for parity 
	for(i = 0;i < 5;i++) //8bits,8bits,8bits,8bits,8bits to be dealed with
	{
		data=0x0;
		for(num = 0;num < 8;num++)  //each bit to be dealed with
		{              
			j = 0;
			while( !gpio_get_value(PIN) )
			{
				udelay(10);
				j++;
				if(j > 10)
					break;
			}
			udelay(28);
			if( gpio_get_value(PIN) ) 
				flag = 0x01; //received bit is 1			
			else
				flag = 0x0; //received bit is 0
			
			//wait until receive low level signal,and run next one bit receive process
			j = 0;
			while( gpio_get_value(PIN) )
			{
				udelay(10);
				j++;
				if(j > 60)
					break;
			}
			data<<=1;   //high significant bit will be received first.
			data|=flag;
		}
		buf[i] = data;
	}
	check_parity = buf[0] + buf[1] + buf[2] + buf[3]; //parity by summing the total 32 bits of temperature and humidity
	if(buf[4] == check_parity) //parity pass
	{
		check_flag = 1;
		printk("humidity check pass\n");
		printk("humidity=[%d],temp=[%d]\n", buf[0], buf[2]);
	}
	else //parity fail
	{
		check_flag = 0;
		printk("humidity check fail\n");           
	}                   
}

static ssize_t humidity_read(struct file *file, char* buffer, size_t size, loff_t *off)
{
    int ret;
	
    humidity_read_data();
		
    if(check_flag == 1)
    {
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
    	return -EAGAIN;
    }
}


static int humidity_open(struct inode *inode, struct file *file)
{
    printk("open in kernel\n");
	int ret;
	// Reserve gpios
	if( gpio_request( PIN, DEVICE_NAME ) < 0 )
	{
		printk( KERN_INFO "%s: %s unable to get gpio\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}
	printk("dht11 gpio request ok!\n");
	// Set gpios directions
	if( gpio_direction_output( PIN, 1) < 0 )
	{
		printk( KERN_INFO "%s: %s unable to set gpio as output\n", DEVICE_NAME, __func__ );
		ret = -EBUSY;
		return(ret);
	}
	printk("dht11 gpio setup ok!\n");
	
    return 0;
}

static int humidity_release(struct inode *inode, struct file *file)
{
    gpio_free(PIN);
    printk("dht11 gpio release\n");
    return 0;
}

static struct file_operations humidity_dev_fops={
    owner		    :	THIS_MODULE,
    open		    :	humidity_open,
	read		    :	humidity_read,
	release		    :	humidity_release,
};
static struct class *dht11_class;

static int __init humidity_dev_init(void) 
{
	int	ret;
	
	printk("init in kernel\n");
	ret = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &humidity_dev_fops);
	if (ret < 0) 
	{
		printk(KERN_INFO "%s: registering device %s with major %d failed with %d\n",
		       __func__, DEVICE_NAME, DEVICE_MAJOR, DEVICE_MAJOR );
		return(ret);
	}
	printk("dht11 driver register success!\n");
	
	dht11_class = class_create(THIS_MODULE, "dht11");
        if (IS_ERR(dht11_class))
	{
		printk(KERN_WARNING "Can't make node %d\n", DEVICE_MAJOR);
        	return PTR_ERR(dht11_class);
	}
        device_create(dht11_class, NULL, MKDEV(DEVICE_MAJOR, 0), NULL, DEVICE_NAME);    
        printk("dht11 driver make node success!\n");
	
        return 0;
}

static void __exit humidity_dev_exit(void)
{
	printk("exit in kernel\n");
        unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
	printk("Remove dht11 device success!\n");
}

module_init(humidity_dev_init);
module_exit(humidity_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("WWW");
