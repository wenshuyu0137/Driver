#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/irq.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>

#define NAME "LED"
#define COUNT 1

static struct led_device{
    dev_t led_id;
    int major,minor;
    int gpio;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    spinlock_t lock;
}led_device;

static int led_open(struct inode *nodep,struct file *filep)
{
    filep->private_data = &led_device;
    return 0;
}

static int led_release(struct inode *nodep,struct file *filep)
{
    return 0;
}
static ssize_t led_write(struct file *filep,const char __user *buf,size_t size, loff_t *loffp)
{
    int ret;
    unsigned long flags;
    unsigned char status;

    struct led_device *dev = filep->private_data;
    spin_lock_irqsave(&dev->lock,flags);
    ret = copy_from_user(&status,buf,size);
    if(ret != 0){
        ret = -EINVAL;
        return ret;
    }
    spin_unlock_irqrestore(&dev->lock,flags);
    if(status == 0){
        gpio_set_value(dev->gpio,0);
    }
    if(status == 1){
        gpio_set_value(dev->gpio,1);
    }

    return 0;
}

struct file_operations led_fop = {
    .open = led_open,
    .release = led_release,
    .write = led_write,
};

static int led_probe(struct platform_device *dev)
{
    int ret;
    pr_info("led probe\n");
    if(led_device.led_id){
        ret = register_chrdev_region(led_device.led_id,COUNT,NAME);
    }else{
        ret = alloc_chrdev_region(&led_device.led_id,0,COUNT,NAME);
        
        led_device.major = MAJOR(led_device.led_id);
        led_device.minor = MINOR(led_device.led_id);
    }
    if(ret<0){
        pr_info("device ID alloc error\n");
        ret = -EINVAL;
        return ret;
    }

    led_device.cdev.owner = THIS_MODULE;
    cdev_init(&led_device.cdev,&led_fop);
    ret = cdev_add(&led_device.cdev,led_device.led_id,COUNT);
    if(ret<0){
        pr_info("cdev add error\n");
        unregister_chrdev_region(led_device.led_id,COUNT);
        ret = -EINVAL;
        return ret;
    }

    led_device.class = class_create(THIS_MODULE,NAME);
    if(IS_ERR(led_device.class)){
        cdev_del(&led_device.cdev);
        pr_info("class creat error\n");
        ret = -EINVAL;
        return ret;
    }
    led_device.device = device_create(led_device.class,NULL,led_device.led_id,NULL,NAME);
    if(IS_ERR(led_device.device)){
        class_destroy(led_device.class);
        pr_info("device creat error\n");
        ret = -EINVAL;
        return ret;
    }

    led_device.node = dev->dev.of_node;

    led_device.gpio = of_get_named_gpio(led_device.node,"led-gpio",0);
    if(!gpio_is_valid(led_device.gpio)){
        pr_info("gpio get error\n");
        ret = -EINVAL;
        return ret;
    }

    ret = gpio_request(led_device.gpio,"led-gpio");
    if(ret){
        pr_info("gpio request error!\n");
        ret = -EINVAL;
        return ret;
    }

    ret = gpio_direction_output(led_device.gpio,0);
    if(ret<0){
        pr_info("gpio mode error\n");
        gpio_free(led_device.gpio);
        ret = -EINVAL;
        return ret;
    }

    spin_lock_init(&led_device.lock);

    return 0;
} 

static int led_remove(struct platform_device *dev)
{
    pr_info("led remove\n");

    gpio_set_value(led_device.gpio,0);
    cdev_del(&led_device.cdev);
    unregister_chrdev_region(led_device.led_id,COUNT);
    device_destroy(led_device.class,led_device.led_id);
    class_destroy(led_device.class);
    gpio_free(led_device.gpio);
    return 0;
}

struct of_device_id led_match_table[] = {
    { .compatible = "fsmp1a, led" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, led_match_table);
struct platform_driver led_driver = {
    .driver = {
        .name = "led",
        .of_match_table = led_match_table,
    },
    .probe = led_probe,
    .remove = led_remove,
};


static int __init led_driver_init(void)
{
    
    return platform_driver_register(&led_driver);
}

static void __exit led_driver_exit(void)
{
    platform_driver_unregister(&led_driver);
}

module_init(led_driver_init);
module_exit(led_driver_exit);
MODULE_LICENSE("GPL");

