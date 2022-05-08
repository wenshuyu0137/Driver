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

#define LED_COUNT 1
#define LED_NAME "led2"

struct gpio_led_device{
    dev_t led_id;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int led_gpio;
};

struct gpio_led_device led;

static int led_open(struct inode *nodep,struct file *filep)
{
    filep->private_data = &led;
    return 0;
}

static int led_release(struct inode *nodep,struct file *filep)
{
    return 0;
}

static ssize_t led_write(struct file *filep,const char __user *buf,size_t count,loff_t *loffp)
{
    int ret;
    unsigned char databuf[1];
    unsigned char arg;
    struct gpio_led_device *led_dev = filep->private_data;
    ret = copy_from_user(databuf,buf,1);
    if(ret<0){
        printk("fail accept arg\n");
        return -EFAULT;
    }
    arg = databuf[0];
    
    if(arg == 0){
        gpio_set_value(led_dev->led_gpio,0); /*led off*/
    }else if(arg == 1){
        gpio_set_value(led_dev->led_gpio,1); /*led on*/
    }
    return 0;
}

static const struct file_operations led_fop = {
    .owner = THIS_MODULE,
    .write = led_write,
    .open = led_open,
    .release = led_release,
};

static int __init gpio_led_init(void)
{
    int ret;

    /*register a device*/
    if(led.major){
        led.led_id = MKDEV(led.major,0);
        ret = register_chrdev_region(led.led_id,LED_COUNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&led.led_id,0,LED_COUNT,LED_NAME);
        led.major = MAJOR(led.led_id);
        led.minor = MINOR(led.led_id);
    }
    if(ret<0){
        goto register_err;
    }
    printk("led device major = %d\n",led.major);
    printk("led device minor = %d\n",led.minor);

    /*add the device*/
    led.cdev.owner = THIS_MODULE;
    cdev_init(&led.cdev,&led_fop);
    ret = cdev_add(&led.cdev,led.led_id,LED_COUNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automatically*/
    led.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(led.class)){
        goto class_cread_err;
    }
    led.device = device_create(led.class,NULL,led.led_id,NULL,LED_NAME);
    if(IS_ERR(led.class)){
        goto device_cread_err;
    }

    /*get dts node*/
    led.node = of_find_node_by_path("/gpio-led");
    if(led.node == NULL){
        goto find_node_err;
    }

    /* get led gpio*/
    led.led_gpio = of_get_named_gpio(led.node,"led-gpio",0);
    if(led.led_gpio < 0){
        goto get_gpio_err;
    }

    /*we must request the gpio before we use it*/
    ret = gpio_request(led.led_gpio,"led-gpio");
    if(ret){
        goto request_err;
    }

    /*use IO*/

    /*set gpio direction to output*/
    ret = gpio_direction_output(led.led_gpio,0);/*low level default*/
    if(ret<0){
        goto direction_set_err;
    }

    /*set value to light led*/

    gpio_set_value(led.led_gpio,1); /*high level*/

    


    return 0;

direction_set_err:
    printk("gpio direction set error\n");
    gpio_free(led.led_gpio);
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
    class_destroy(led.class);
    return PTR_ERR(led.device);
class_cread_err:
    cdev_del(&led.cdev);
    return PTR_ERR(led.class);
cdev_add_err:
    printk("cdev add error");
    unregister_chrdev_region(led.led_id,LED_COUNT);
register_err:
    printk("register device error\n");
    return -EINVAL;
}

static void __exit gpio_led_exit(void)
{
    /*release sourse*/
    gpio_set_value(led.led_gpio,0); /*led off*/
    cdev_del(&led.cdev);
    unregister_chrdev_region(led.led_id,LED_COUNT);

    device_destroy(led.class,led.led_id);
    class_destroy(led.class);

    gpio_free(led.led_gpio);
}

module_init(gpio_led_init);
module_exit(gpio_led_exit);
MODULE_LICENSE("GPL");

