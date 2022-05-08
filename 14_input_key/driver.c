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
#include <linux/miscdevice.h>
#include <linux/input.h>
#include <linux/irq.h>

#define KEY_NAME "KEY"
#define KEY_COUNT 1
#define TIMER_PERIOD 20
/*device infomaton*/
struct dev_info{
    int gpio,irq ;
    unsigned long irq_flags;
    struct device_node *node ;          /*device node*/
    struct timer_list key_time;         /*timer*/
    struct tasklet_struct key_tasklet;  /*bottom half irq*/
    struct input_dev *key_input;        /*input system*/

    irqreturn_t (*handler)(int,void*);  /*irq function*/
    void (*func)(unsigned long);        /*tasklet function*/
}my_key;


/*platform match table*/
struct of_device_id key_input_of_match[] = {
    { .compatible = "fsmp1a, key" },
    {/*sentinel*/},
};

static irqreturn_t key_irq_handler(int irq,void *dev)
{
    struct dev_info *key_dev = dev;
    tasklet_schedule(&key_dev->key_tasklet);
    return IRQ_HANDLED;
}

static void key_tasklet(unsigned long addr)
{
    struct dev_info *key_dev = container_of((struct tasklet_struct*)addr,struct dev_info,key_tasklet);
    mod_timer(&key_dev->key_time,key_dev->key_time.expires);
}

static void timer_function(struct timer_list *t)
{
    unsigned long status;
    status = gpio_get_value(my_key.gpio);
    if(status == 0){
        input_event(my_key.key_input,EV_KEY,KEY_0,0);
        input_sync(my_key.key_input);
    }else if(status == 1){
        input_event(my_key.key_input,EV_KEY,KEY_0,1);
        input_sync(my_key.key_input);
    }
}

static int key_input_probe(struct platform_device *dev)
{
    int ret;

    my_key.node = dev->dev.of_node;     /*get node*/
    if(my_key.node == NULL){
        pr_info("node get fail\n");
        ret = -EINVAL;
        return ret;
    }

    my_key.gpio = of_get_named_gpio(my_key.node,"key-gpio",0);    /*get gpio*/
    if(!gpio_is_valid(my_key.gpio)){
        pr_info("gpio get fail\n");
        ret = -EINVAL;
        return ret;
    }
    pr_info("gpio = %d\n",my_key.gpio);
    
    ret = gpio_request(my_key.gpio,KEY_NAME); /*request gpio*/
    if(ret<0){
        pr_info("gpio request fail\n");
        ret = -EINVAL;
        return ret;
    }

    ret = gpio_direction_input(my_key.gpio);  /*set gpio mode*/
    if(ret<0){
        pr_info("gpio mode set fail\n");
        ret = -EINVAL;
        return ret;
    }

    my_key.key_time.expires = jiffies+msecs_to_jiffies(TIMER_PERIOD);   /*init timer period*/
    timer_setup(&my_key.key_time,timer_function,0);     /*init timer*/
    
    my_key.irq = gpio_to_irq(my_key.gpio); /*get irq from gpio*/
    pr_info("irq = %d\n",my_key.irq);

    my_key.irq_flags = irq_get_trigger_type(my_key.irq);    /*get irq trigger mode*/

    my_key.handler = key_irq_handler;
    my_key.func = key_tasklet;
    ret = request_irq(my_key.irq,my_key.handler,my_key.irq_flags,KEY_NAME,&my_key);   /*request irq*/
    if(ret<0){
        pr_info("irq get fail\n");
        ret = -EINVAL;
        return ret;
    }

    tasklet_init(&my_key.key_tasklet,my_key.func,(unsigned long)&my_key.key_tasklet);


    my_key.key_input = input_allocate_device();     /*request a input device*/
    if(my_key.key_input == NULL){
        pr_info("input device alloc fail\n");
        ret = -EINVAL;
        return ret;
    }

    /*init input device*/
    my_key.key_input->name = KEY_NAME;
    __set_bit(EV_KEY,my_key.key_input->evbit); /*set to key event*/
    __set_bit(EV_REP,my_key.key_input->evbit); /*set to repeat event*/
    __set_bit(KEY_0,my_key.key_input->keybit); /*set key ID*/

    ret = input_register_device(my_key.key_input);
    if(ret<0){
        pr_info("input register fail\n");
        ret = -EINVAL;
        return ret;
    }


    return ret;
}

static int key_input_remove(struct platform_device *dev)
{
    free_irq(my_key.irq,&my_key);
    gpio_free(my_key.gpio);
    del_timer_sync(&my_key.key_time);
    input_unregister_device(my_key.key_input);

    //input_free_device(my_key.key_input); /*it will creat a bug ,don't know why*/

    return 0;
}
/*platform driver*/
struct platform_driver key_input_driver = {
    .driver = {
        .name = "fsmp1a, key",
        .of_match_table = key_input_of_match,
    },
    .probe = key_input_probe,
    .remove = key_input_remove,
};

static int __init key_input_init(void)
{
    return platform_driver_register(&key_input_driver);
}

static void __exit key_input_exit(void)
{
    platform_driver_unregister(&key_input_driver);
}



module_init(key_input_init);
module_exit(key_input_exit);
MODULE_LICENSE("GPL");

