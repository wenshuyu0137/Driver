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

#define MISC_NAME "misc_beep"
#define MISC_MINOR 255
/*device struct*/
static struct device_info{
    struct device_node *nd;
    int gpio;
    int minor;
}beep;

static int beep_open(struct inode *nodep,struct file *filep)
{   
    filep->private_data = &beep;
    return 0;
}

static int beep_release(struct inode *nodep,struct file *filep)
{   
    return 0;
}

static ssize_t beep_write(struct file *filep,const char __user *buf,size_t size,loff_t *loffp)
{
    int ret;
    unsigned long status;
    struct device_info *dev = filep->private_data;
    ret = copy_from_user(&status,buf,sizeof(buf));
    if(ret<0){
        pr_info("value get fail\n");
        ret = -EINVAL;
        return ret;
    }

    if(status == 0){
        gpio_set_value(dev->gpio,0);
    }else if(status == 1){
        gpio_set_value(dev->gpio,1);
    }
    return 0;
}
static struct file_operations beep_fop = {
    .owner  =THIS_MODULE,
    .open = beep_open,
    .write = beep_write,
    .release = beep_release,
};

/*misc device*/
static struct miscdevice beep_misc = {
    .name = MISC_NAME,
    .minor = MISC_MINOR,
    .fops = &beep_fop,
};

/*platform match table*/
struct of_device_id beep_of_match[] = {
    { .compatible = "stm32fsmp1a-beep" },
    {/*sentinel*/},
};

/*platform probe*/
static int beep_probe(struct platform_device *dev)
{
    int ret;
    /*get dts node*/
    beep.nd = dev->dev.of_node;

    /*get gpio*/
    beep.gpio = of_get_named_gpio(beep.nd,"beep-gpio",0);
    if(beep.gpio<0){
        pr_info("gpio find fail\n");
        ret = -EINVAL;
        return ret;
    }
    /*request gpio*/
    ret = gpio_request(beep.gpio,"beep-gpio");
    if(ret){
        pr_info("request gpio fail\n");
        ret = -EINVAL;
        return ret;
    }
    /*set gpio mode*/
    ret = gpio_direction_output(beep.gpio,0);
    if(ret<0){
        pr_info("gpio mode set fail\n");
        gpio_free(beep.gpio);
        ret = -EINVAL;
        return ret;
    }
    /*register misc device*/
    ret = misc_register(&beep_misc);
    if(ret<0){
        pr_info("misc register fail\n");
        ret = -EINVAL;
        return ret;
    }
    return 0;
}

/*platform remove*/
static int beep_remove(struct platform_device *dev)
{
    gpio_set_value(beep.gpio,0);
    gpio_free(beep.gpio);
    misc_deregister(&beep_misc);
    return 0;
}
/*platform driver*/
struct platform_driver beep_driver = {
    .driver = {
        .name = "stm32fsmp1a-beep",
        .of_match_table = beep_of_match,
    },
    .probe = beep_probe,
    .remove = beep_remove,
    
};

static int __init misc_beep_init(void)
{
    return platform_driver_register(&beep_driver);
}

static void __exit misc_beep_exit(void)
{

    platform_driver_unregister(&beep_driver);
}



module_init(misc_beep_init);
module_exit(misc_beep_exit);
MODULE_LICENSE("GPL");

