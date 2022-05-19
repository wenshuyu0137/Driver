#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bitfield.h>
#include <linux/counter.h>
#include <linux/iio/iio.h>
#include <linux/mfd/stm32-lptimer.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "AP3216C_reg.h"

#define DEVICE_NAME             "AP3216C"

static struct of_device_id AP3216C_match_table[] = {
    {.compatible = "wsy_ap3216c"},
    {/* sentinel */},
};

/*no device tree*/
static struct i2c_device_id AP3216C_id_table[] = {
    {"wsy_ap3216c",0},
    {/* sentinel */},
};

struct AP3216C_device{
    struct i2c_client *client;
    struct regmap *regmap;
    struct regmap_config config;
    struct mutex lock;
};

#define AP3216C_CHANEL(_TYPE){ \
        .type = _TYPE,               \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE) ,    \
}

static const struct iio_chan_spec i2c_channels[] = {
    
    AP3216C_CHANEL(IIO_INTENSITY),
    AP3216C_CHANEL(IIO_PROXIMITY),
    AP3216C_CHANEL(IIO_LIGHT),
};

static int ap3216c_read_chanel_scale(struct iio_dev *indio_dev,const struct iio_chan_spec *i2c_channels,int *val2)
{ 
    int ret;
    unsigned int buf;
    struct AP3216C_device *dev = iio_priv(indio_dev);
    switch(i2c_channels->type){
        case IIO_INTENSITY:
            regmap_read(dev->regmap,0x10,&buf);
            if((buf&0x30) == 0x00){
                *val2 = 0.35*1000000;
            }else if((buf&0x30) == 0x10){
                *val2 = 0.0788*1000000;
            }
            else if((buf&0x30) == 0x20){
                *val2 = 0.0179*1000000;
            }else if((buf&0x30) == 0x30){
                *val2 = 0.0049*1000000;
            }else{
                pr_info("read fail\n");
            }
            break;
        case IIO_PROXIMITY:
            pr_info("don't know\n");
            break;
        case IIO_LIGHT:
            pr_info("don't know\n");
            break;
        default:
            ret = -EINVAL;
            break;
    }  
    return ret;
}
static int ap3216c_read_chanel_raw(struct iio_dev *indio_dev,const struct iio_chan_spec *i2c_channels,int *val)
{ 
    int ret;
    struct AP3216C_device *dev = iio_priv(indio_dev);
    unsigned int buf[2];
    switch(i2c_channels->type){
        case IIO_LIGHT:
            regmap_read(dev->regmap,IR_Data_Low,&buf[0]);
            regmap_read(dev->regmap,IR_Data_High,&buf[1]);
            if((buf[0] & 0x80) == 0x80){
                *val = 0;
            }else{
                *val = (buf[1]<<2) | (buf[0] & 0x03);
            }   
            //pr_info("red light(ir)\n");        
            break;

        case IIO_INTENSITY:
            regmap_read(dev->regmap,ALS_Data_Low,&buf[0]);
            regmap_read(dev->regmap,ALS_Data_High,&buf[1]);
            *val = (buf[1]<<8) | buf[0];
            //pr_info("guang qiang du(ALS)\n"); 
            break;

        case IIO_PROXIMITY:
            regmap_read(dev->regmap,PS_Data_Low,&buf[0]);
            regmap_read(dev->regmap,PS_Data_High,&buf[1]);
            if((buf[0] & 0x40) == 0x40){
                *val = 0;
            }else{
                *val = ((buf[1]&0x3f)<<4) | (buf[0]&0x0f);
            }
            //pr_info("jie jin ju li(PS)\n");

            break;
        default:
            ret = -EINVAL;
            break;
    }  
    return ret;
}

static int ap3216c_read_raw(struct iio_dev *indio_dev,struct iio_chan_spec const *chan,int *val,int *val2,long mask)
{
    int ret;
    struct AP3216C_device *dev = iio_priv(indio_dev);

    /*red different mask data*/
    switch(mask){
        case IIO_CHAN_INFO_RAW:
            mutex_lock(&dev->lock);
            ap3216c_read_chanel_raw(indio_dev,chan,val);
            mutex_unlock(&dev->lock);
            return IIO_VAL_INT;
        case IIO_CHAN_INFO_SCALE:
            mutex_lock(&dev->lock);
            *val = 0;
            ap3216c_read_chanel_scale(indio_dev,chan,val2);
            mutex_unlock(&dev->lock);
            return IIO_VAL_INT_PLUS_MICRO;
        default:
            ret = -EINVAL;
            break;
    }
    return ret;
}

static int ap3216c_write_raw_get_fmt(struct iio_dev *indio_dev,struct iio_chan_spec const *chan,long mask)
{
    /*red different mask data*/
    switch(mask){
        case IIO_CHAN_INFO_SCALE:
            return IIO_VAL_INT_PLUS_MICRO;
        default:
            return IIO_VAL_INT_PLUS_MICRO;
    }
}

static int ap3216c_write_chanel_scale(struct iio_dev *indio_dev,const struct iio_chan_spec *i2c_channels,int val2)
{ 
    int ret;
    static int i;
    unsigned int buf;
    struct AP3216C_device *dev = iio_priv(indio_dev);
    regmap_read(dev->regmap,0x10,&buf);
    pr_info("val2 = %d\n",val2);
    pr_info("buf = %#x\n",buf);
    pr_info("i = %d\n",i++);
    switch(i2c_channels->type){
        case IIO_LIGHT:
            pr_info("don't know\n");       
            break;
        case IIO_INTENSITY:
            switch(val2){
                case 350000:
                    regmap_write(dev->regmap,(unsigned char)0x10,((buf|0x30) & (~0x30)));
                    break;
                case 78800:
                    regmap_write(dev->regmap,(unsigned char)0x10,((buf|0x30) & (~0x20)));
                    break;
                case 17900:
                    regmap_write(dev->regmap,(unsigned char)0x10,((buf|0x30) & (~0x10)));
                    break;
                case 4900:
                    regmap_write(dev->regmap,(unsigned char)0x10,((buf|0x30) & (~0x00)));
                    break;
                default:
                    break;
            }
            break;
        case IIO_PROXIMITY:
            pr_info("don't know\n");       
            break;
        default:
            ret = -EINVAL;
            break;
    }  
    return ret;
}
static int ap3216c_write_raw(struct iio_dev *indio_dev,struct iio_chan_spec const *chan,int val,int val2,long mask)
{  
    int ret;
    struct AP3216C_device *dev = iio_priv(indio_dev);

    /*write different mask data*/
    switch(mask){
        case IIO_CHAN_INFO_SCALE:
            mutex_lock(&dev->lock);
            ap3216c_write_chanel_scale(indio_dev,chan,val2);
            mutex_unlock(&dev->lock);
            return IIO_VAL_INT_PLUS_MICRO;
        default:
            ret = -EINVAL;
    }
    return ret;
}

static const struct iio_info AP3216C_info = {
    .read_raw = ap3216c_read_raw,
    .write_raw = ap3216c_write_raw,
    .write_raw_get_fmt = ap3216c_write_raw_get_fmt,
};



static int AP3216C_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct AP3216C_device *pri;
    struct iio_dev *indio_dev;

    indio_dev = devm_iio_device_alloc(&client->dev,sizeof(*pri));
    if(!indio_dev){
        return -ENOMEM;
    }
    pri = iio_priv(indio_dev);
    i2c_set_clientdata(client,indio_dev);
    pri->client = client;
    mutex_init(&pri->lock);
 
    /*init IIO */
    indio_dev->dev.parent = &client->dev;
    indio_dev->channels = i2c_channels;
    indio_dev->num_channels = ARRAY_SIZE(i2c_channels);
    indio_dev->name = DEVICE_NAME;
    indio_dev->modes = INDIO_DIRECT_MODE;     /*supply sys interface*/
    indio_dev->info = &AP3216C_info;

    /*register to kernel*/
    ret = iio_device_register(indio_dev);
    if(ret<0){
        pr_info("register iio device fail\n");
        return -EINVAL;
    }
    
    pri->config.reg_bits = 8;
    pri->config.val_bits = 8;
    pri->regmap = regmap_init_i2c(client,&pri->config);
    if(IS_ERR(pri->regmap)){
        iio_device_unregister(indio_dev);
        ret =  PTR_ERR(pri->regmap);
        return ret;
    }

    regmap_write(pri->regmap,System_Configuration,0x04);
    mdelay(300);
    regmap_write(pri->regmap,System_Configuration,0x03);
    mdelay(300);
    return 0;
}

static int AP3216C_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct AP3216C_device *pri;

    pri = iio_priv(indio_dev);

    regmap_exit(pri->regmap);
    iio_device_unregister(indio_dev);

    return 0;
}

static struct i2c_driver ap3216c_driver = {
    .probe = AP3216C_probe,
    .remove = AP3216C_remove,
    .driver = {
        .name = "AP3216C",
        .owner = THIS_MODULE,
        .of_match_table = AP3216C_match_table,
    },
    .id_table = AP3216C_id_table,   /*no device tree*/
};

static int __init ap3216c_init(void)
{
    return i2c_add_driver(&ap3216c_driver);
}

static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);
MODULE_LICENSE("GPL");

