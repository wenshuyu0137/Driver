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
#include <linux/poll.h>

#define COUNT 1
#define NAME "MY_DEV"
#define KEY_NUM 1

enum key_status{
    key_press = 0,
    key_release,
    key_keep,
};

struct key_desc {
    int gpio;                                   /*gpio id*/
    int irq;                                    /*irq id*/

    char name[10];                              /*key name*/

    unsigned long flags;                        /*Trigger mode*/

    irqreturn_t (*handler)(int,void*);          /*irq function*/
    void (*tasklet_func)(unsigned long);                /*tasklet function*/

};

struct timer_desc {
    volatile unsigned long period;
};

struct device_desc{
    dev_t dev_id;
    atomic_t status;
    wait_queue_head_t r_wait;
 

    int major;
    int minor;

    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *node;
    struct key_desc my_key[KEY_NUM];
    struct timer_list timer;
    struct timer_desc time_args;
    struct tasklet_struct my_tasklet;           /*tasklet*/

}my_device;


static int dev_open(struct inode *nodep,struct file *filep)
{
    filep->private_data = &my_device;
    return 0;
}

static int dev_release(struct inode *nodep,struct file *filep)
{
    return 0;
}

static ssize_t dev_read(struct file *filep,char __user *buf,size_t count,loff_t *loffp)
{
    int ret = 0;

    struct device_desc *dev = filep->private_data;

    if(filep->f_flags & O_NONBLOCK){
        ret = wait_event_interruptible(dev->r_wait,atomic_read(&dev->status) != key_keep);
        if(ret){
            return ret;
        }
    }else{
        if(atomic_read(&dev->status) == key_keep){
            return -EAGAIN;
        } 
    }

    ret = copy_to_user(buf,&dev->status,sizeof(int));

    atomic_set(&dev->status,key_keep);
 

    return ret;
}

static unsigned int dev_poll (struct file *filep, struct poll_table_struct *wait)
{
    int ret;
    struct device_desc *dev = filep->private_data;
    poll_wait(filep,&dev->r_wait,wait);

    if(atomic_read(&dev->status) == (key_press || key_release)){
        ret = POLLIN | POLLRDNORM;
    }
    return ret;
}

static const struct file_operations dev_fop = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .open = dev_open,
    .release = dev_release,
    .poll = dev_poll,
};

static void timer_function(struct timer_list *t)
{
    unsigned char value;
    struct device_desc *key_dev = &my_device;

    value = gpio_get_value(key_dev->my_key[0].gpio);
    if(value == 0){
        atomic_set(&key_dev->status,key_press);
        wake_up_interruptible(&key_dev->r_wait);   
    }else if(value == 1){
        atomic_set(&key_dev->status,key_release);
        wake_up_interruptible(&key_dev->r_wait); 
    }else{
        atomic_set(&key_dev->status,key_keep);
    }


}

static irqreturn_t key0_irq_handler(int irq, void* dev)
{
    int i;
    struct device_desc *key_dev = dev;

    for(i=0;i<KEY_NUM;i++){
        tasklet_schedule(&key_dev->my_tasklet);
    }

    return IRQ_HANDLED;
}

static void tasklet_function(unsigned long addr)
{
    struct device_desc *tasklet_dev = container_of((struct tasklet_struct *)addr, struct device_desc, my_tasklet);

    mod_timer(&tasklet_dev->timer,tasklet_dev->timer.expires);
}

static int init_irq(struct device_desc *dev)
{
    int ret,i,j;
    struct device_desc *my_dev = dev;
    /*1.get irq id*/
    for(i=0;i<KEY_NUM;i++){ 

        /*get irq number*/
        my_dev->my_key[i].irq = gpio_to_irq(my_dev->my_key[i].gpio);  
        /*dev->my_key[i].irq = irq_of_parse_and_map(dev->node, i);*/        /*universal method of get irq*/

            if(my_dev->my_key[i].irq<0){
                goto Universal_err;
            }

            /*get irq trigger mode*/
            my_dev->my_key[0].flags = irq_get_trigger_type(my_dev->my_key[i].irq);
            if(my_dev->my_key[0].flags == IRQF_TRIGGER_NONE){
                my_dev->my_key[0].flags = IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING; 
            }
    }
    
    /*choose irq function(top half)*/
    my_dev->my_key[0].handler = key0_irq_handler;
    my_dev->my_key[0].tasklet_func = tasklet_function;

    /*2. irq init*/
    for(i=0;i<KEY_NUM;i++){                 /*request irq(top half)*/
        ret = request_irq(my_dev->my_key[i].irq, my_dev->my_key[i].handler, my_dev->my_key->flags, my_dev->my_key[i].name, my_dev);
        if(ret<0){
            for(j=0;j<i;j++){               /*if request error Release previous gpio*/
                free_irq(my_dev->my_key[j].irq,my_dev);
            }
            goto irq_request_err;
        }
        printk("irq id = %d",my_dev->my_key[i].irq);

        /*bottom half init*/
        tasklet_init(&my_dev->my_tasklet, my_dev->my_key[i].tasklet_func, (unsigned long)(&my_dev->my_tasklet));
    }

    


    return ret;

irq_request_err:
    for(i=0;i<KEY_NUM;i++){
        gpio_free(my_dev->my_key[i].gpio);
    }
    return -EINVAL;
Universal_err:
    ret = -EINVAL;
    return ret;
}

static void init_timer(struct device_desc *dev)
{
    struct device_desc *my_key = dev;
    /*init timer*/
    timer_setup(&my_key->timer,timer_function,0);

    /*set period*/
    my_key->time_args.period = 15;
    my_key->timer.expires = jiffies+msecs_to_jiffies(my_key->time_args.period);

    /*add_timer(&my_key->timer);*/
}

static int init_key(struct device_desc *dev)
{
    int ret,i;
    /*1. GPIO init*/

        /*get node*/
    dev->node = of_find_node_by_path("/key");
    if(dev->node == NULL){
        goto Universal_err;
    }
        /*get gpio id*/
    for(i=0;i<KEY_NUM;i++){
        dev->my_key[i].gpio = of_get_named_gpio(dev->node, "key-gpio", i);
        if(dev->my_key[i].gpio<0){
            goto Universal_err;
        }
        printk("key[%d] gpio = %d",i,dev->my_key[i].gpio);
    }
        

        /*request gpio and set mode*/
    for(i=0;i<KEY_NUM;i++){

        memset(dev->my_key[i].name, 0, sizeof(dev->my_key[i].name));
        sprintf(dev->my_key[i].name, "key[%d]", i);                         /*set key name*/

        ret = gpio_request(dev->my_key[i].gpio, dev->my_key[i].name);       /*request*/
        if(ret<0){
            goto Universal_err;
        }

        ret = gpio_direction_input(dev->my_key[i].gpio);                    /*set mode*/
        if(ret<0){
            goto gpio_mode_set_err;
        }
    }

    return 0;

gpio_mode_set_err:
    gpio_free(dev->my_key[i].gpio);
    return -EINVAL;

Universal_err:
    ret = -EINVAL;
    return ret;
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
    printk("device major = %d\n",my_device.major);
    printk("device minor = %d\n",my_device.minor);

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
    printk("cdev add error");
    unregister_chrdev_region(my_device.dev_id,COUNT);
register_err:
    printk("register device error\n");
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


    /*init key*/
    ret = init_key(&my_device);
    if(ret<0){
        return -EINVAL;
    }

    /*init timer*/
    init_timer(&my_device);

    /*init wait queue head*/
    init_waitqueue_head(&my_device.r_wait);

    /*init atomic status*/
    my_device.status = (atomic_t)ATOMIC_INIT(key_keep);

    /*init irq*/
    ret = init_irq(&my_device);
    if(ret<0){
        return -EINVAL;
    }


    return 0;

}

static void __exit dev_exit(void)
{   
    int i;

    /*release irq*/
    for(i=0;i<KEY_NUM;i++){
        free_irq(my_device.my_key[i].irq,&my_device);
    }

    /*release gpio*/
    for(i=0;i<KEY_NUM;i++){
        gpio_free(my_device.my_key[i].gpio);
    }
 
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

