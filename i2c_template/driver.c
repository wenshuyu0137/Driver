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
#include "xxx.h"

static struct of_device_id xxx_match_table[] = {
    {.compatible = "xxx"},
    {/* sentinel */},
};

/*no device tree*/
static struct i2c_device_id xxx_id_table[] = {
    {"xxx",0},
    {/* sentinel */},
};

struct i2c_client xxx_client = {
    
};

static int xxx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    return 0;
}

static int xxx_remove(struct i2c_client *client)
{
    return 0;
}

static struct i2c_driver ap3216_driver = {
    .probe = xxx_probe,
    .remove = xxx_remove,
    .driver = {
        .name = "xxx",
        .owner = THIS_MODULE,
        .of_match_table = xxx_match_table,
    },
    .id_table = xxx_id_table,   /*no device tree*/
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

