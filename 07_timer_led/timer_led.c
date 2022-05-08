#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>

#define LED_COUNT 1
#define LED_NAME "LED"

#define CLOSE_CMD           _IO(0xEF, 1)
#define OPEN_CMD            _IO(0xEF, 2)
#define SETCYCLE_CMD        _IOW(0xEF, 3, int)

struct timer_device{
    dev_t timer_id;
    int major;
    int minor;
    int led_gpio;
    int cycle;    /*ms*/
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    struct timer_list my_timer;
    spinlock_t lock;
};

struct timer_device timer;

static void timer_function(struct timer_list *t)
{
    static int status = 1;
    status = !status;
    gpio_set_value(timer.led_gpio,status);

    /*restart timer*/
    mod_timer(&timer.my_timer,jiffies+msecs_to_jiffies(timer.cycle));
}
static int timer_open(struct inode *nodep,struct file *filep)
{
    filep->private_data = &timer;
    return 0;
}

static int timer_release(struct inode *nodep,struct file *filep)
{
    return 0;
}

static long timer_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int ret,value;
    unsigned long flags;
    struct timer_device *dev = filep->private_data;
    switch (cmd){

    case CLOSE_CMD: 
        del_timer_sync(&dev->my_timer);
        break;
    case OPEN_CMD:
        mod_timer(&timer.my_timer,jiffies+msecs_to_jiffies(timer.cycle));
        break;

    case SETCYCLE_CMD:     
        ret = copy_from_user(&value,(unsigned int *)arg,sizeof(arg));
        if(ret<0){
            return -EFAULT;
        }

        spin_lock_irqsave(&dev->lock,flags);
        dev->cycle = value;
        spin_unlock_irqrestore(&dev->lock,flags);

        del_timer_sync(&dev->my_timer);
        mod_timer(&timer.my_timer,jiffies+msecs_to_jiffies(dev->cycle));
        
        break;

    }

    return 0;
}


static const struct file_operations timer_fop = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = timer_ioctl,
    .open = timer_open,
    .release = timer_release,
};

static int __init gpio_timer_init(void)
{
    int ret;

    /*register a device*/
    if(timer.major){
        timer.timer_id = MKDEV(timer.major,0);
        ret = register_chrdev_region(timer.timer_id,LED_COUNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&timer.timer_id,0,LED_COUNT,LED_NAME);
        timer.major = MAJOR(timer.timer_id);
        timer.minor = MINOR(timer.timer_id);
    }
    if(ret<0){
        goto register_err;
    }
    printk("timer device major = %d\n",timer.major);
    printk("timer device minor = %d\n",timer.minor);

    /*add the device*/
    timer.cdev.owner = THIS_MODULE;
    cdev_init(&timer.cdev,&timer_fop);
    ret = cdev_add(&timer.cdev,timer.timer_id,LED_COUNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automatically*/
    timer.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(timer.class)){
        goto class_cread_err;
    }
    timer.device = device_create(timer.class,NULL,timer.timer_id,NULL,LED_NAME);
    if(IS_ERR(timer.class)){
        goto device_cread_err;
    }

    /*get dts node*/
    timer.node = of_find_node_by_path("/gpio-led");
    if(timer.node == NULL){
        goto find_node_err;
    }

    /* get led gpio*/
    timer.led_gpio = of_get_named_gpio(timer.node,"led-gpio",0);
    if(timer.led_gpio < 0){
        goto get_gpio_err;
    }

    /*we must request the gpio before we use it*/
    ret = gpio_request(timer.led_gpio,"led-gpio");
    if(ret){
        goto request_err;
    }

    /*use IO*/

    /*set gpio direction to output*/
    ret = gpio_direction_output(timer.led_gpio,0);/*low level default*/
    if(ret<0){
        goto direction_set_err;
    }

    /*set value to light led*/
    gpio_set_value(timer.led_gpio,1); /*high level*/

    /*init my_timer*/
    timer_setup(&timer.my_timer,timer_function,0);

    timer.cycle = 500;
    timer.my_timer.expires = jiffies+(msecs_to_jiffies(timer.cycle)); /*set timer 500ms*/

    /*register timer and add to system*/
    add_timer(&timer.my_timer);


    return 0;

direction_set_err:
    printk("gpio direction set error\n");
    gpio_free(timer.led_gpio);
    ret = -EINVAL;
    return ret;
request_err:
    printk("gpio request err\n");
    ret = -EINVAL;
    return ret;
get_gpio_err:
    printk("can't find gpio\n");
    ret = -EINVAL;
    return ret;
find_node_err:
    ret = -EINVAL;
    return ret;
device_cread_err:
    class_destroy(timer.class);
    return PTR_ERR(timer.device);
class_cread_err:
    cdev_del(&timer.cdev);
    return PTR_ERR(timer.class);
cdev_add_err:
    printk("cdev add error");
    unregister_chrdev_region(timer.timer_id,LED_COUNT);
register_err:
    printk("register device error\n");
    return -EINVAL;
}

static void __exit gpio_timer_exit(void)
{
    /*del timer*/
    del_timer_sync(&timer.my_timer);

    /*release sourse*/
    gpio_set_value(timer.led_gpio,0); /*timer off*/
    cdev_del(&timer.cdev);
    unregister_chrdev_region(timer.timer_id,LED_COUNT);

    device_destroy(timer.class,timer.timer_id);
    class_destroy(timer.class);

    gpio_free(timer.led_gpio);
}

module_init(gpio_timer_init);
module_exit(gpio_timer_exit);
MODULE_LICENSE("GPL");

