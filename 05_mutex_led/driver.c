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
#define LED_NAME "mutex_led"

struct mutex_led{
    dev_t ledid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int gpio;

    struct mutex lock;

}mutex_led;

static int mutex_led_open(struct inode *inodep, struct file *filep)
{
    
    filep->private_data = &mutex_led;

    mutex_lock(&mutex_led.lock);    
    
    return 0;
}

static int mutex_led_release(struct inode *inodep, struct file *filep)
{
    
    struct mutex_led *dev = filep->private_data;

    mutex_unlock(&dev->lock);

    return 0;
}

static ssize_t mutex_led_write(struct file *filep, const char __user *buf, size_t cnt, loff_t *loffp)
{   
    int ret;
    unsigned char databuf[1];
    unsigned char data;
    struct mutex_led *dev = filep->private_data;



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
    .open = mutex_led_open,
    .write = mutex_led_write,
    .release = mutex_led_release,
};

static int __init mutex_led_init(void)
{
    int ret;

    /*init a mutex*/
    mutex_init(&mutex_led.lock);
    
    /* register a device */
    if(mutex_led.major){
        mutex_led.ledid = MKDEV(mutex_led.major,mutex_led.minor);
        ret = register_chrdev_region(mutex_led.ledid,LED_CNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&mutex_led.ledid,0,LED_CNT,LED_NAME);
        mutex_led.major = MAJOR(mutex_led.ledid);
        mutex_led.minor = MINOR(mutex_led.ledid);
    }
    if(ret<0){
        goto register_err;
    }

    /* add the device */
    cdev_init(&mutex_led.cdev,&led_fop);
    ret = cdev_add(&mutex_led.cdev,mutex_led.ledid,LED_CNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automaticallt*/
    mutex_led.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(mutex_led.class)){
        goto class_creat_err;
    }
    mutex_led.device = device_create(mutex_led.class,NULL,mutex_led.ledid,NULL,LED_NAME);
    if(IS_ERR(mutex_led.class)){
        goto device_creat_err;
    }

    /* get gpio node */
    mutex_led.node = of_find_node_by_path("/ato-led");
    if(mutex_led.node == NULL){
        goto get_goio_node_err;
    }

    /* get led gpio*/
    mutex_led.gpio = of_get_named_gpio(mutex_led.node,"led-gpio",0);
    if(mutex_led.gpio<0){
        goto get_gpio_err;
    }

    /*request gpio*/
    ret = gpio_request(mutex_led.gpio,"led-gpio");
    if(ret<0){
        goto request_err;
    }

    /*use gpio*/
    ret = gpio_direction_output(mutex_led.gpio,0); /*dafault low level to off led*/
    if(ret<0){
        goto gpio_set_output_err;
    }

    return 0;


gpio_set_output_err:
    printk("gpio set output error\n");
    gpio_free(mutex_led.gpio);
    return -EINVAL;
request_err:
    printk("gpio request error\n");
    return -EINVAL;
get_gpio_err:
    printk("get gpio error\n");
    return -EINVAL;
get_goio_node_err:
    device_destroy(mutex_led.class,mutex_led.ledid);
    printk("gpio node get error\n");
device_creat_err:
    class_destroy(mutex_led.class);
    return PTR_ERR(mutex_led.device);
class_creat_err:
    cdev_del(&mutex_led.cdev);
    return PTR_ERR(mutex_led.class);
cdev_add_err:
    printk("device add error\n");
    unregister_chrdev_region(mutex_led.ledid,LED_CNT);
register_err:
    printk("device register error\n");
    ret = -EINVAL;
    return ret;
}

static void __exit mutex_led_exit(void)
{
    gpio_set_value(mutex_led.gpio,0); /*off led*/

    cdev_del(&mutex_led.cdev);
    unregister_chrdev_region(mutex_led.ledid,LED_CNT);

    device_destroy(mutex_led.class,mutex_led.ledid);
    class_destroy(mutex_led.class);
    

    gpio_free(mutex_led.gpio);
}


 module_init(mutex_led_init);
 module_exit(mutex_led_exit);
 MODULE_LICENSE("GPL");
