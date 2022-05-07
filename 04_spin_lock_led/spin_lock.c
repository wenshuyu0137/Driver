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


#define LED_CNT 1
#define LED_NAME "spin_led"

struct spin_led{
    dev_t ledid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int gpio;

    int status; 
    spinlock_t lock;
}spin_led;

static int spin_led_open(struct inode *inodep, struct file *filep)
{
    unsigned long flags;
    filep->private_data = &spin_led;

    spin_lock_irqsave(&spin_led.lock,flags);
    if(spin_led.status){
        spin_unlock_irqrestore(&spin_led.lock,flags);
        return -EBUSY;
    }

    spin_led.status++;
    spin_unlock_irqrestore(&spin_led.lock,flags);

    return 0;
}

static int spin_led_release(struct inode *inodep, struct file *filep)
{
    unsigned long flags;

    struct spin_led *dev = filep->private_data;

    spin_lock_irqsave(&dev->lock,flags);
    if(dev->status){
        dev->status--;
    }
    spin_unlock_irqrestore(&dev->lock,flags);



    return 0;
}

static ssize_t spin_led_write(struct file *filep, const char __user *buf, size_t cnt, loff_t *loffp)
{   
    int ret;
    unsigned char databuf[1];
    unsigned char data;
    struct spin_led *dev = filep->private_data;



    ret = copy_from_user(databuf,buf,1);
    if(ret<0){
        printk("recieve data error\n");
        return -EFAULT;
    }

    data = databuf[0];

    if(data == 0){
        gpio_set_value(dev->gpio,0); /*off led*/
    }
    else if(data == 1){
        gpio_set_value(dev->gpio,1); /*light led*/
    }

    return 0;
}

struct file_operations led_fop = {
    .owner  = THIS_MODULE,
    .open = spin_led_open,
    .write = spin_led_write,
    .release = spin_led_release,
};

static int __init spin_led_init(void)
{
    int ret;

    /*init a spin lock*/
    spin_lock_init(&spin_led.lock);
    spin_led.status = 0;
    
    /* register a device */
    if(spin_led.major){
        spin_led.ledid = MKDEV(spin_led.major,spin_led.minor);
        ret = register_chrdev_region(spin_led.ledid,LED_CNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&spin_led.ledid,0,LED_CNT,LED_NAME);
        spin_led.major = MAJOR(spin_led.ledid);
        spin_led.minor = MINOR(spin_led.ledid);
    }
    if(ret<0){
        goto register_err;
    }

    /* add the device */
    cdev_init(&spin_led.cdev,&led_fop);
    ret = cdev_add(&spin_led.cdev,spin_led.ledid,LED_CNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automaticallt*/
    spin_led.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(spin_led.class)){
        goto class_creat_err;
    }
    spin_led.device = device_create(spin_led.class,NULL,spin_led.ledid,NULL,LED_NAME);
    if(IS_ERR(spin_led.class)){
        goto device_creat_err;
    }

    /* get gpio node */
    spin_led.node = of_find_node_by_path("/ato-led");
    if(spin_led.node == NULL){
        goto get_goio_node_err;
    }

    /* get led gpio*/
    spin_led.gpio = of_get_named_gpio(spin_led.node,"led-gpio",0);
    if(spin_led.gpio<0){
        goto get_gpio_err;
    }

    /*request gpio*/
    ret = gpio_request(spin_led.gpio,"led-gpio");
    if(ret<0){
        goto request_err;
    }

    /*use gpio*/
    ret = gpio_direction_output(spin_led.gpio,0); /*dafault low level to off led*/
    if(ret<0){
        goto gpio_set_output_err;
    }

    return 0;


gpio_set_output_err:
    printk("gpio set output error\n");
    gpio_free(spin_led.gpio);
    return -EINVAL;
request_err:
    printk("gpio request error\n");
    return -EINVAL;
get_gpio_err:
    printk("get gpio error\n");
    return -EINVAL;
get_goio_node_err:
    device_destroy(spin_led.class,spin_led.ledid);
    printk("gpio node get error\n");
device_creat_err:
    class_destroy(spin_led.class);
    return PTR_ERR(spin_led.device);
class_creat_err:
    cdev_del(&spin_led.cdev);
    return PTR_ERR(spin_led.class);
cdev_add_err:
    printk("device add error\n");
    unregister_chrdev_region(spin_led.ledid,LED_CNT);
register_err:
    printk("device register error\n");
    ret = -EINVAL;
    return ret;
}

static void __exit spin_led_exit(void)
{
    gpio_set_value(spin_led.gpio,0); /*off led*/

    cdev_del(&spin_led.cdev);
    unregister_chrdev_region(spin_led.ledid,LED_CNT);

    device_destroy(spin_led.class,spin_led.ledid);
    class_destroy(spin_led.class);
    

    gpio_free(spin_led.gpio);
}


 module_init(spin_led_init);
 module_exit(spin_led_exit);
 MODULE_LICENSE("GPL");