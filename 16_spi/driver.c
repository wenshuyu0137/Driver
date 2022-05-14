#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/spi/spi.h>
#include <linux/string.h>

#define DEVICE_NAME             "m74hc595"

static struct of_device_id m74hc595_match_table[] = {
    {.compatible = "m74hc595"},
    {/* sentinel */},
};

/*no device tree*/
static struct spi_device_id m74hc595_id_table[] = {
    {"m74hc595",0},
    {/* sentinel */},
};

struct m74hc595_device{
    dev_t id;
    struct cdev cdev;
    struct class *class;
    struct device *device;

    struct device_node *nd;
    int css_gpio;

    void *privata_data;
}m74hc595_dev;


static int m74hc595_OPEN(struct inode *np,struct file *filp)
{
    return 0;
}

static ssize_t m74hc595_READ(struct file *filp,char __user *buf,size_t size,loff_t *loffp)
{
    unsigned char data[2];

    struct spi_device *spi = (struct spi_device *)m74hc595_dev.privata_data;

    spi_read(spi, data, 2);
    copy_to_user(buf,data,2);

    return 0;
}

static ssize_t m74hc595_WRITE(struct file *filp,const char __user *buf,size_t size,loff_t *loffp)
{   
    unsigned char data[4];
    int ret,i;
    unsigned char reg[4] = {
        [3] = 1 << 3,
        [2] = 1 << 2,
        [1] = 1 << 1,
        [0] = 1 << 0,
    };
    unsigned char value[2];
    struct spi_device *spi = (struct spi_device *)m74hc595_dev.privata_data;

    ret = copy_from_user(data,buf,sizeof(data));

    if(ret<0){
        pr_info("recieve data fail\n");
        ret = -EINVAL;
        return ret;
    }

    for(i=0;i<4;i++){
        value[0] = reg[i];
        value[1] = data[i];
        spi_write(spi, value, 2);
    }

    return 0;
}

static int m74hc595_RELEASE(struct inode *np,struct file *filp)
{
    struct spi_device *spi = (struct spi_device *)m74hc595_dev.privata_data;
    unsigned char value[2];
    value[0] = 1 << 3;
    value[1] = 0x3F;
    spi_write(spi, value, 2);

    return 0;
}

static const struct file_operations fop = {
    .open = m74hc595_OPEN,
    .write = m74hc595_WRITE,
    .release = m74hc595_RELEASE,
    .read  = m74hc595_READ,
};

static int m74hc595_probe(struct spi_device *spi)
{
    int ret;
    pr_info("m74hc595 device probe success\n");

    if(m74hc595_dev.id){
        ret = register_chrdev_region(m74hc595_dev.id,1,DEVICE_NAME);
    }else{
        ret = alloc_chrdev_region(&m74hc595_dev.id,0,1,DEVICE_NAME);
    }
    if(ret<0){
        pr_info("device register fail\n");
    }else{
        pr_info("dev major = %d,dev minor = %d\n",MAJOR(m74hc595_dev.id),MINOR(m74hc595_dev.id));
    }

    cdev_init(&m74hc595_dev.cdev,&fop);
    ret = cdev_add(&m74hc595_dev.cdev,m74hc595_dev.id,1);
    if(ret<0){
        pr_info("cdev add fail\n");
        unregister_chrdev_region(m74hc595_dev.id,1);
        ret = -EINVAL;
        return ret;
    }

    m74hc595_dev.class = class_create(THIS_MODULE,DEVICE_NAME);
    if(IS_ERR(m74hc595_dev.class)){
        pr_info("class creat fail\n");
        cdev_del(&m74hc595_dev.cdev);
        ret = -EINVAL;
        return ret;
    }
    m74hc595_dev.device = device_create(m74hc595_dev.class,NULL,m74hc595_dev.id,NULL,DEVICE_NAME);
    if(IS_ERR(m74hc595_dev.device)){
        pr_info("class creat fail\n");
        class_destroy(m74hc595_dev.class);
        ret = -EINVAL;
        return ret;
    }

    spi->mode = SPI_MODE_0;
    spi_setup(spi);

    m74hc595_dev.privata_data = spi;


    return ret;
}

static int m74hc595_remove(struct spi_device *spi)
{
    gpio_direction_output(m74hc595_dev.css_gpio,1);

    gpio_free(m74hc595_dev.css_gpio);

    cdev_del(&m74hc595_dev.cdev);

    unregister_chrdev_region(m74hc595_dev.id,1);

    device_destroy(m74hc595_dev.class,m74hc595_dev.id);

    class_destroy(m74hc595_dev.class);
    return 0;
}

static struct spi_driver m74hc595_driver = {
    .probe = m74hc595_probe,
    .remove = m74hc595_remove,
    .driver = {
        .name = "m74hc595",
        .owner = THIS_MODULE,
        .of_match_table = m74hc595_match_table,
    },
    .id_table = m74hc595_id_table,   /*no device tree*/
};

static int __init m74hc595_init(void)
{
    return spi_register_driver(&m74hc595_driver);
}

static void __exit m74hc595_exit(void)
{
    spi_unregister_driver(&m74hc595_driver);
}



module_init(m74hc595_init);
module_exit(m74hc595_exit);
MODULE_LICENSE("GPL");

