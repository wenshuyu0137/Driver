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
#include <linux/atomic.h>

#define LED_CNT 1
#define LED_NAME "ato_led"

struct atomic_led{
    dev_t ledid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    atomic_t sem;
    int gpio;
    atomic_t lock;
}ato_led;

static int ato_led_open(struct inode *inodep, struct file *filep)
{
    filep->private_data = &ato_led;

    if(!atomic_dec_and_test(&ato_led.lock)){
        atomic_inc(&ato_led.lock);
        return -EBUSY;
    }


    return 0;
}

static int ato_led_release(struct inode *inodep, struct file *filep)
{
    struct atomic_led *dev = filep->private_data;

    atomic_inc(&dev->lock); /*release lock*/

    return 0;
}

static ssize_t ato_led_write(struct file *filep, const char __user *buf, size_t cnt, loff_t *loffp)
{   
    int ret;
    unsigned char databuf[1];
    unsigned char data;
    struct atomic_led *dev = filep->private_data;

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
    .open = ato_led_open,
    .write = ato_led_write,
    .release = ato_led_release,
};

static int __init atomic_led_init(void)
{
    int ret;

    ato_led.lock = (atomic_t)ATOMIC_INIT(0); /*init the lock*/
    atomic_set(&ato_led.lock,1);   /*set the lock value to 1*/
    
    /* register a device */
    if(ato_led.major){
        ato_led.ledid = MKDEV(ato_led.major,ato_led.minor);
        ret = register_chrdev_region(ato_led.ledid,LED_CNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&ato_led.ledid,0,LED_CNT,LED_NAME);
        ato_led.major = MAJOR(ato_led.ledid);
        ato_led.minor = MINOR(ato_led.ledid);
    }
    if(ret<0){
        goto register_err;
    }

    /* add the device */
    cdev_init(&ato_led.cdev,&led_fop);
    ret = cdev_add(&ato_led.cdev,ato_led.ledid,LED_CNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automaticallt*/
    ato_led.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(ato_led.class)){
        goto class_creat_err;
    }
    ato_led.device = device_create(ato_led.class,NULL,ato_led.ledid,NULL,LED_NAME);
    if(IS_ERR(ato_led.class)){
        goto device_creat_err;
    }

    /* get gpio node */
    ato_led.node = of_find_node_by_path("/ato-led");
    if(ato_led.node == NULL){
        goto get_goio_node_err;
    }

    /* get led gpio*/
    ato_led.gpio = of_get_named_gpio(ato_led.node,"led-gpio",0);
    if(ato_led.gpio<0){
        goto get_gpio_err;
    }

    /*request gpio*/
    ret = gpio_request(ato_led.gpio,"led-gpio");
    if(ret<0){
        goto request_err;
    }

    /*use gpio*/
    ret = gpio_direction_output(ato_led.gpio,0); /*dafault low level to off led*/
    if(ret<0){
        goto gpio_set_output_err;
    }

    return 0;


gpio_set_output_err:
    printk("gpio set output error\n");
    gpio_free(ato_led.gpio);
    return -EINVAL;
request_err:
    printk("gpio request error\n");
    return -EINVAL;
get_gpio_err:
    printk("get gpio error\n");
    return -EINVAL;
get_goio_node_err:
    device_destroy(ato_led.class,ato_led.ledid);
    printk("gpio node get error\n");
device_creat_err:
    class_destroy(ato_led.class);
    return PTR_ERR(ato_led.device);
class_creat_err:
    cdev_del(&ato_led.cdev);
    return PTR_ERR(ato_led.class);
cdev_add_err:
    printk("device add error\n");
    unregister_chrdev_region(ato_led.ledid,LED_CNT);
register_err:
    printk("device register error\n");
    ret = -EINVAL;
    return ret;
}

static void __exit atomic_led_exit(void)
{
    gpio_set_value(ato_led.gpio,0); /*off led*/

    cdev_del(&ato_led.cdev);
    unregister_chrdev_region(ato_led.ledid,LED_CNT);

    device_destroy(ato_led.class,ato_led.ledid);
    class_destroy(ato_led.class);
    

    gpio_free(ato_led.gpio);
}


 module_init(atomic_led_init);
 module_exit(atomic_led_exit);
 MODULE_LICENSE("GPL");