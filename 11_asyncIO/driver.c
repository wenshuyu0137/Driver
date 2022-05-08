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
#include <linux/fcntl.h>
#include <linux/poll.h>

#define COUNT 1
#define NAME "MY_DEV"
#define KEY_INVALUE 0xFF
#define KEEP -1

struct key_desc {
    int gpio;                                   /*gpio id*/
    int irq;                                    /*irq id*/
    char name[10];                              /*key name*/
    unsigned long flags;                        /*Trigger mode*/
    irqreturn_t (*handler)(int,void*);          /*irq function*/
};

struct timer_desc {
    volatile unsigned long period;
};

struct device_desc{
    dev_t dev_id;
    wait_queue_head_t r_wait;

    int major;
    int minor;
    int gpio;
    unsigned char status;

    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    struct key_desc my_key;
    struct timer_list timer;
    struct timer_desc time_args;
    struct tasklet_struct my_tasklet;
    struct fasync_struct *my_async;

    atomic_t gpio_value;
}my_device;

static int dev_async(int fd, struct file *filep, int on)
{
    struct device_desc *dev = filep->private_data;
    return fasync_helper(fd,filep,on,&dev->my_async);
}

static int dev_open(struct inode *nodep,struct file *filep)
{
    filep->private_data = &my_device;
    wait_event_interruptible(my_device.r_wait,(my_device.status == 0 || my_device.status == 1));

    return 0;
}

static int dev_release(struct inode *nodep,struct file *filep)
{
    //return 0;
    return dev_async(-1, filep, 0);
}

static ssize_t dev_read(struct file *filep,char __user *buf,size_t count,loff_t *loffp)
{
    int ret;
    struct device_desc *dev = filep->private_data;

    //wait_event_interruptible(dev->r_wait,(dev->status == 0 || dev->status == 1));

/*
    if(filep->f_flags & O_NONBLOCK){
        wait_event_interruptible(dev->r_wait,(dev->status == 0 || dev->status == 1));    
    }else{
        pr_info("please open file at NONBLOCK mode");
        return -EAGAIN;
    }
*/
    
    ret = copy_to_user(buf,&dev->status,sizeof(dev->status));

    dev->status = KEEP;
    return ret;
}
/*
static unsigned int dev_poll(struct file * filep, struct poll_table_struct *wait)
{
    int ret;
    struct device_desc *dev = filep->private_data;
    poll_wait(filep,&dev->r_wait,wait);
    if(dev->status == 0 || dev->status == 1){
        ret = POLLIN || POLLRDNORM;
    }
    return ret;

}
*/
static const struct file_operations dev_fop = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .open = dev_open,
    .release = dev_release,
    .fasync = dev_async,
    //.poll = dev_poll,
};

static void timer_function(struct timer_list *t)
{  
    struct device_desc *timer_dev = &my_device;

    atomic_set(&timer_dev->gpio_value,gpio_get_value(timer_dev->gpio));
    
    if(atomic_read(&timer_dev->gpio_value) == 0){
        timer_dev->status = 0;
        wake_up_interruptible(&timer_dev->r_wait);
        if(timer_dev->my_async){
            kill_fasync(&timer_dev->my_async,SIGIO, POLL_IN);
        }
    }else if(atomic_read(&timer_dev->gpio_value) == 1){
        timer_dev->status = 1;
        wake_up_interruptible(&timer_dev->r_wait);
        if(timer_dev->my_async){
            kill_fasync(&timer_dev->my_async,SIGIO, POLL_IN);
        }
    }else{
        timer_dev->status = -1;
    }
}

static irqreturn_t key_irq_handler(int irq, void* dev)
{
    struct device_desc *irq_dev = dev;
    tasklet_schedule(&irq_dev->my_tasklet);
    
    return IRQ_HANDLED;
}

static void tasklet_funcion(unsigned long addr)
{
    struct device_desc *tkl_dev = container_of((struct tasklet_struct *)addr, struct device_desc, my_tasklet);
    
    mod_timer(&tkl_dev->timer,tkl_dev->time_args.period);
}

static int init_irq(struct device_desc *dev)
{
    struct device_desc *irq_dev = dev;

    irq_dev->my_key.irq = gpio_to_irq(irq_dev->gpio);
    if(irq_dev->my_key.irq<0){
        pr_info("get irq err\n");
        return -EINVAL;
    }

    irq_dev->my_key.flags = irq_get_trigger_type(irq_dev->my_key.irq);
    if(irq_dev->my_key.flags == IRQF_TRIGGER_NONE){
        irq_dev->my_key.flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING; 
    }

    memset(irq_dev->my_key.name,0,sizeof(irq_dev->my_key.name));
    sprintf(irq_dev->my_key.name,"key_irq");
    irq_dev->my_key.handler = key_irq_handler;

    request_irq(irq_dev->my_key.irq,irq_dev->my_key.handler,irq_dev->my_key.flags,irq_dev->my_key.name,irq_dev);

    tasklet_init(&irq_dev->my_tasklet,tasklet_funcion,(unsigned long)(&irq_dev->my_tasklet));

    return 0;
}

static void init_timer(struct device_desc *dev)
{
    struct device_desc *timer_dev = dev;
    /*init timer*/
    timer_setup(&timer_dev->timer,timer_function,0);

    /*set period*/
    timer_dev->time_args.period = 20;
    timer_dev->timer.expires = jiffies+msecs_to_jiffies(timer_dev->time_args.period);

    //add_timer(&timer_dev->timer);

}

static int init_key(struct device_desc *dev)
{
    int err;
    struct device_desc *gpio_dev = dev;
    gpio_dev->node = of_find_node_by_path("/key"); 
    if(gpio_dev->node == NULL){
        pr_info("get gpio node err\n");
        return -EINVAL;
    }

    gpio_dev->gpio = of_get_named_gpio(gpio_dev->node,"key-gpio",0);
    if(gpio_dev->gpio<0){
        pr_info("get gpio id err\n");
        return -EINVAL;
    }
    
    err = gpio_request(gpio_dev->gpio,NAME);
    if(err<0){
        pr_info("request gpio fail\n");
        return -EINVAL;
    }

    err = gpio_direction_input(gpio_dev->gpio);
    if(err<0){
        pr_info("gpio mode set err fail\n");
        return -EINVAL;
    }

    gpio_dev->status = KEEP;

    return 0;
}

static int device_init(void)
{
    /*init device*/
    int ret;
    /*register a device*/
    if(my_device.major){
        my_device.dev_id = MKDEV(my_device.major,0);
        ret = register_chrdev_region(my_device.dev_id,COUNT,NAME);
    }else{
        ret = alloc_chrdev_region(&my_device.dev_id,0,COUNT,NAME);
        my_device.major = MAJOR(my_device.dev_id);
        my_device.minor = MINOR(my_device.dev_id);
    }
    if(ret<0){
        goto register_err;
    }
    pr_info("device major = %d\n",my_device.major);
    pr_info("device minor = %d\n",my_device.minor);

    /*add the device*/
    my_device.cdev.owner = THIS_MODULE;
    cdev_init(&my_device.cdev,&dev_fop);
    ret = cdev_add(&my_device.cdev,my_device.dev_id,COUNT);
    if(ret<0){
        goto cdev_add_err;
    }

    /*set add device automatically*/
    my_device.class = class_create(THIS_MODULE,NAME);
    if(IS_ERR(my_device.class)){
        goto class_cread_err;
    }
    my_device.device = device_create(my_device.class,NULL,my_device.dev_id,NULL,NAME);
    if(IS_ERR(my_device.class)){
        goto device_cread_err;
    }

    return ret;

device_cread_err:
    class_destroy(my_device.class);
    return PTR_ERR(my_device.device);
class_cread_err:
    cdev_del(&my_device.cdev);
    return PTR_ERR(my_device.class);
cdev_add_err:
    pr_info("cdev add error");
    unregister_chrdev_region(my_device.dev_id,COUNT);
register_err:
    pr_info("register device error\n");
    return -EINVAL;
}

static int __init dev_init(void)
{
    int ret;

    /*init device*/
    ret = device_init();
    if(ret<0){
        return -EINVAL;
    }

    /*init atomic*/
    my_device.gpio_value = (atomic_t)ATOMIC_INIT(0);

    /*init queue head*/
    init_waitqueue_head(&my_device.r_wait);

    /*init key*/
    ret = init_key(&my_device);
    if(ret<0){
        return -EINVAL;
    }

    /*init timer*/
    init_timer(&my_device);

    /*init irq*/
    ret = init_irq(&my_device);
    if(ret<0){
        return -EINVAL;
    }


    return 0;

}

static void __exit dev_exit(void)
{   
    free_irq(my_device.my_key.irq,&my_device);
 

    /*release gpio*/

    gpio_free(my_device.my_key.gpio);

 
    /*release timer*/
    del_timer_sync(&my_device.timer);

    cdev_del(&my_device.cdev);
    unregister_chrdev_region(my_device.dev_id,COUNT);

    device_destroy(my_device.class,my_device.dev_id);
    class_destroy(my_device.class);
  
}

module_init(dev_init);
module_exit(dev_exit);
MODULE_LICENSE("GPL");

