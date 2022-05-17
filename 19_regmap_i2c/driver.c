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
#include <linux/i2c.h>
#include <linux/regmap.h>
#include "AP3216C_reg.h"

#define DEVICE_NAME             "AP3216C"

static struct of_device_id AP3216C_match_table[] = {
    {.compatible = "liteon,ap3216c"},
    {/* sentinel */},
};

/*no device tree*/
static struct i2c_device_id AP3216C_id_table[] = {
    {"liteon,ap3216c",0},
    {/* sentinel */},
};

struct i2c_client AP3216C_client = {
    
};

struct AP3216C_device{
    dev_t id;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    void *private_data;
    unsigned short ir, als, ps;

    struct regmap *regmap;
    struct regmap_config config;

}AP3216C_dev;

static int AP3216C_read_regs(struct AP3216C_device *dev, u8 reg)
{
    unsigned int data;

    regmap_read(dev->regmap,reg,&data);

    return data;
}

static void AP3216C_write_regs(struct AP3216C_device *dev, u8 reg, unsigned char val)
{

    regmap_write(dev->regmap,reg,val);

}

void AP3216C_readdata(struct AP3216C_device *dev)
{
	unsigned char i =0;
	unsigned char buf[6];

	
	for(i = 0; i < 6; i++) {
		buf[i] = AP3216C_read_regs(dev, IR_Data_Low + i); 
	}
	if(buf[0] & 0X80) 
	dev->ir = 0; 
	else 
	dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0X03); 

	dev->als = ((unsigned short)buf[3] << 8) | buf[2]; 

	if(buf[4] & 0x40) 
	dev->ps = 0; 
	else 
	dev->ps = ((unsigned short)(buf[5] & 0X3F) << 4) | (buf[4] &0X0F);
}

static int AP3216C_OPEN(struct inode *np,struct file *filp)
{

    unsigned char value;
    filp->private_data = &AP3216C_dev;

    /*init AP3216C*/
    AP3216C_write_regs(filp->private_data,System_Configuration,0x4);    /*reset bit*/
    mdelay(200);
    AP3216C_write_regs(filp->private_data,System_Configuration,0x3);    /*ALS and PS+IR functions active*/
    mdelay(300);
    value = AP3216C_read_regs(filp->private_data, System_Configuration);
    pr_info("read value = %#x\n",value);

    return 0;
}

static ssize_t AP3216C_READ(struct file *filp,char __user *buf,size_t size,loff_t *loffp)
{   
    short data[3];
    long err = 0;
    struct AP3216C_device *dev = &AP3216C_dev;

    pr_info("AP3216C read\n");
    AP3216C_readdata(dev);
    data[0] = dev->ir;
    data[1] = dev->als;
    data[2] = dev->ps;
    err = copy_to_user(buf, data, sizeof(data));
    return 0;
}

static int AP3216C_RELEASE(struct inode *np,struct file *filp)
{
    pr_info("AP3216C close\n");
    return 0;
}

static const struct file_operations fop = {
    .open = AP3216C_OPEN,
    .read = AP3216C_READ,
    .release = AP3216C_RELEASE,
};

static int AP3216C_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    pr_info("AP3216C device probe success\n");

    if(AP3216C_dev.id){
        ret = register_chrdev_region(AP3216C_dev.id,1,DEVICE_NAME);
    }else{
        ret = alloc_chrdev_region(&AP3216C_dev.id,0,1,DEVICE_NAME);
    }
    if(ret<0){
        pr_info("device register fail\n");
    }else{
        pr_info("dev major = %d,dev minor = %d\n",MAJOR(AP3216C_dev.id),MINOR(AP3216C_dev.id));
    }

    cdev_init(&AP3216C_dev.cdev,&fop);
    ret = cdev_add(&AP3216C_dev.cdev,AP3216C_dev.id,1);
    if(ret<0){
        pr_info("cdev add fail\n");
        unregister_chrdev_region(AP3216C_dev.id,1);
        ret = -EINVAL;
        return ret;
    }

    AP3216C_dev.class = class_create(THIS_MODULE,DEVICE_NAME);
    if(IS_ERR(AP3216C_dev.class)){
        pr_info("class creat fail\n");
        cdev_del(&AP3216C_dev.cdev);
        ret = -EINVAL;
        return ret;
    }
    AP3216C_dev.device = device_create(AP3216C_dev.class,NULL,AP3216C_dev.id,NULL,DEVICE_NAME);
    if(IS_ERR(AP3216C_dev.device)){
        pr_info("class creat fail\n");
        class_destroy(AP3216C_dev.class);
        ret = -EINVAL;
        return ret;
    }

    AP3216C_dev.config.reg_bits = 8;
    AP3216C_dev.config.val_bits = 8;
    AP3216C_dev.regmap = regmap_init_i2c(client,&AP3216C_dev.config);

    AP3216C_dev.private_data = client;

    return ret;
}

static int AP3216C_remove(struct i2c_client *client)
{
    cdev_del(&AP3216C_dev.cdev);

    unregister_chrdev_region(AP3216C_dev.id,1);

    device_destroy(AP3216C_dev.class,AP3216C_dev.id);

    class_destroy(AP3216C_dev.class);

    regmap_exit(AP3216C_dev.regmap);

    return 0;
}

static struct i2c_driver ap3216_driver = {
    .probe = AP3216C_probe,
    .remove = AP3216C_remove,
    .driver = {
        .name = "AP3216C",
        .owner = THIS_MODULE,
        .of_match_table = AP3216C_match_table,
    },
    .id_table = AP3216C_id_table,   /*no device tree*/
};

static int __init xxx_init(void)
{
    return i2c_add_driver(&ap3216_driver);
}

static void __exit xxx_exit(void)
{
    i2c_del_driver(&ap3216_driver);
}



module_init(xxx_init);
module_exit(xxx_exit);
MODULE_LICENSE("GPL");

