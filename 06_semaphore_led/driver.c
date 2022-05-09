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
#define LED_NAME "semaphore_led"

struct semaphore_led{
    dev_t ledid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int gpio;

    struct semaphore sem;
}semaphore_led;

static int semaphore_led_open(struct inode *inodep, struct file *filep)
{
    
    filep->private_data = &semaphore_led;

    down(&semaphore_led.sem);
    
    return 0;
}

static int semaphore_led_release(struct inode *inodep, struct file *filep)
{
    
    struct semaphore_led *dev = filep->private_data;

    up(&dev->sem);

    return 0;
}

static ssize_t semaphore_led_write(struct file *filep, const char __user *buf, size_t cnt, loff_t *loffp)
{   
    int ret;
    unsigned char databuf[1];
    unsigned char data;
    struct semaphore_led *dev = filep->private_data;



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
    .open = semaphore_led_open,
    .write = semaphore_led_write,
    .release = semaphore_led_release,
};

static int __init semaphore_led_init(void)
{
    int ret;

    /*init a semaphore*/
    sema_init(&semaphore_led.sem,1);
    
    /* register a device */
    if(semaphore_led.major){
        semaphore_led.ledid = MKDEV(semaphore_led.major,semaphore_led.minor);
        ret = register_chrdev_region(semaphore_led.ledid,LED_CNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&semaphore_led.ledid,0,LED_CNT,LED_NAME);
        semaphore_led.major = MAJOR(semaphore_led.ledid);
        semaphore_led.minor = MINOR(semaphore_led.ledid);
    }
    if(ret<0){
        goto register_err;
    }

    /* add the device */
    cdev_init(&semaphore_led.cdev,&led_fop);
    ret = cdev_add(&semaphore_led.cdev,semaphore_led.ledid,LED_CNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automaticallt*/
    semaphore_led.class = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(semaphore_led.class)){
        goto class_creat_err;
    }
    semaphore_led.device = device_create(semaphore_led.class,NULL,semaphore_led.ledid,NULL,LED_NAME);
    if(IS_ERR(semaphore_led.class)){
        goto device_creat_err;
    }

    /* get gpio node */
    semaphore_led.node = of_find_node_by_path("/ato-led");
    if(semaphore_led.node == NULL){
        goto get_goio_node_err;
    }

    /* get led gpio*/
    semaphore_led.gpio = of_get_named_gpio(semaphore_led.node,"led-gpio",0);
    if(semaphore_led.gpio<0){
        goto get_gpio_err;
    }

    /*request gpio*/
    ret = gpio_request(semaphore_led.gpio,"led-gpio");
    if(ret<0){
        goto request_err;
    }

    /*use gpio*/
    ret = gpio_direction_output(semaphore_led.gpio,0); /*dafault low level to off led*/
    if(ret<0){
        goto gpio_set_output_err;
    }

    return 0;


gpio_set_output_err:
    printk("gpio set output error\n");
    gpio_free(semaphore_led.gpio);
    return -EINVAL;
request_err:
    printk("gpio request error\n");
    return -EINVAL;
get_gpio_err:
    printk("get gpio error\n");
    return -EINVAL;
get_goio_node_err:
    device_destroy(semaphore_led.class,semaphore_led.ledid);
    printk("gpio node get error\n");
device_creat_err:
    class_destroy(semaphore_led.class);
    return PTR_ERR(semaphore_led.device);
class_creat_err:
    cdev_del(&semaphore_led.cdev);
    return PTR_ERR(semaphore_led.class);
cdev_add_err:
    printk("device add error\n");
    unregister_chrdev_region(semaphore_led.ledid,LED_CNT);
register_err:
    printk("device register error\n");
    ret = -EINVAL;
    return ret;
}

static void __exit semaphore_led_exit(void)
{
    gpio_set_value(semaphore_led.gpio,0); /*off led*/

    cdev_del(&semaphore_led.cdev);
    unregister_chrdev_region(semaphore_led.ledid,LED_CNT);

    device_destroy(semaphore_led.class,semaphore_led.ledid);
    class_destroy(semaphore_led.class);
    

    gpio_free(semaphore_led.gpio);
}


 module_init(semaphore_led_init);
 module_exit(semaphore_led_exit);
 MODULE_LICENSE("GPL");
