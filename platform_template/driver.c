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

/*platform match table*/
struct of_device_id xxx_of_match[] = {
    { .compatible = "" },
    {/*sentinel*/},
};

static int xxx_probe(struct platform_device *dev)
{
    return 0;
}

static int xxx_remove(struct platform_device *dev)
{
    return 0;
}
/*platform driver*/
struct platform_driver xxx_driver = {
    .driver = {
        .name = "",
        .of_match_table = xxx_of_match,
    },
    .probe = xxx_probe,
    .remove = xxx_remove,
    
};

static int __init xxx_init(void)
{
    return platform_driver_register(&xxx_driver);
}

static void __exit xxx_exit(void)
{
    platform_driver_unregister(&xxx_driver);
}



module_init(xxx_init);
module_exit(xxx_exit);
MODULE_LICENSE("GPL");

