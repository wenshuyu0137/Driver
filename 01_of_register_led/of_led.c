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

#define LED_COUNT 1
#define LED_NAME "my_led"

u32 *GPIOZ_MODER;
u32 *GPIOZ_OTYPER;
u32 *GPIOZ_PUPDR;
u32 *GPIOZ_BSRR;

struct led_dev{
    dev_t led_id;
    int led_major;
    int led_minor;
    struct class *led_node;
    struct device *led_device;
    struct cdev cdev;
    struct device_node *led_dts_node;
};

struct led_dev led;

static int led_open(struct inode *inode, struct file *filep)
{
    return 0;
}

static int led_release(struct inode *inode, struct file *filep)
{
    return 0;
}

static void led_switch(int status)
{
    int val;
    if (status == 0){
        printk("led status = %d\n",status);
        val = readl(GPIOZ_BSRR);
        val |= (1 << 5);
        writel(val,GPIOZ_BSRR);
    }else if(status == 1){
        printk("led status = %d\n",status);
        val = readl(GPIOZ_BSRR);
        val |= (1 << 21);
        writel(val,GPIOZ_BSRR);
    }
}
static ssize_t led_write(struct file* filep,const char __user *buffer,size_t size,loff_t *loffp)
{
    int retval;
    unsigned char databuf[1];
    unsigned char status;
    
    retval = copy_from_user(databuf,buffer,size);
    if(retval<0){
        printk("kernel write fail\n");
        return -EFAULT;
    }
    
    status = databuf[0];

    led_switch(status);
    return 0;

}


static const struct file_operations led_fop = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .release = led_release,
};


static int __init led_init(void)
{
    int ret,i;
    unsigned int val ;
    const char *compatible,*status;
    u32 reg[8];
    /* register a char device*/
    if(led.led_major){
        led.led_id = MKDEV(led.led_major,0);
        ret = register_chrdev_region(led.led_id,LED_COUNT,LED_NAME);
    }else{
        ret = alloc_chrdev_region(&led.led_id,0,LED_COUNT,LED_NAME);
        led.led_major = MAJOR(led.led_id);
        led.led_minor = MINOR(led.led_id);
    }

    if(ret<0){
        printk("fail register!\n");
        goto fail_register;
    }

    /*add a device*/
    led.cdev.owner = THIS_MODULE;
    cdev_init(&led.cdev,&led_fop);
    ret = cdev_add(&led.cdev,led.led_id,LED_COUNT);
    if(ret<0){
        printk("cdev add error");
        goto fail_cdev_add;
    }

    /*auto creat a device node*/
    led.led_node = class_create(THIS_MODULE,LED_NAME);
    if(IS_ERR(led.led_node)){
        ret = PTR_ERR(led.led_node);
        goto fail_creat_class;
    }
    led.led_device = device_create(led.led_node,NULL,led.led_id,NULL,LED_NAME);
    if(IS_ERR(led.led_device)){
        ret = PTR_ERR(led.led_device);
        goto fail_creat_device;
    }

    /*get dts attributr*/
    led.led_dts_node = of_find_node_by_path("/fsmp1a-led");
    if(led.led_dts_node == NULL){
        goto fail_get_dts_node;
    }
    
    ret = of_property_read_string(led.led_dts_node,"compatible",&compatible);
    if(ret<0){
        goto fail_get_string;
    }else{
        printk("compatible = %s\n",compatible);
    }

    ret = of_property_read_string(led.led_dts_node,"status",&status);
    if(ret<0){
        goto fail_get_string;
    }else{
        printk("status = %s\n",status);
    }

    ret = of_property_read_u32_array(led.led_dts_node,"reg",reg,8); 
    if(ret<0){
        goto fail_read_array;
    }else{
        for(i=0;i<8;i++){
            printk("array[%d] = %#x",i,reg[i]);
        }   
        printk("count = %d",sizeof(reg));
    }

    /*  init led*/
    GPIOZ_MODER = of_iomap(led.led_dts_node,0);
    GPIOZ_OTYPER = of_iomap(led.led_dts_node,1);
    GPIOZ_PUPDR = of_iomap(led.led_dts_node,2);
    GPIOZ_BSRR = of_iomap(led.led_dts_node,3);

    val = readl(GPIOZ_MODER);
    val &= ~(0x3 << 10); /*reset bits*/
    val |= (0x1 << 10);  /*set PZ5 to General purpose output mode */
    writel(val,GPIOZ_MODER);

    val = readl(GPIOZ_OTYPER);
    val &= ~(1 << 5); /*reset bits*/
    writel(val,GPIOZ_OTYPER); /*set PZ5 to Output push-pull (reset state) */

    val = readl(GPIOZ_PUPDR);
    val &= ~(0x3 << 10); /*reset bits*/
    val |= (0x1 << 10);  /*set PZ5 to Pull-up */
    writel(val,GPIOZ_PUPDR);

    val = readl(GPIOZ_BSRR);
    val |= (1 << 21);
    writel(val,GPIOZ_BSRR); /* default low level*/

    return 0;

fail_read_array:
    printk("read array error");
fail_get_string:
    printk("get string error");
fail_get_dts_node:
    device_destroy(led.led_node,led.led_id);
    ret = -EINVAL;
    return ret;
fail_creat_device:
    class_destroy(led.led_node);
fail_creat_class:
    cdev_del(&led.cdev);
fail_cdev_add:
    unregister_chrdev_region(led.led_id,LED_COUNT);
fail_register:
    ret = -EINVAL;
    return ret;

}  

static void __exit led_exit(void)
{
    /* release sourse*/
    int val;
    val = readl(GPIOZ_BSRR);
    val |= (1 << 5);
    writel(val,GPIOZ_BSRR);
    iounmap(GPIOZ_MODER);
    iounmap(GPIOZ_OTYPER);
    iounmap(GPIOZ_PUPDR);
    iounmap(GPIOZ_BSRR);
    cdev_del(&led.cdev);
    unregister_chrdev_region(led.led_id,LED_COUNT);
    device_destroy(led.led_node,led.led_id);
    class_destroy(led.led_node);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
