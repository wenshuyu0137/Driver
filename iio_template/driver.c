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

#define DEVICE_NAME             "xxxxxx"

static struct of_device_id xxxxxx_match_table[] = {
    {.compatible = "xxxxxx"},
    {/* sentinel */},
};

/*no device tree*/
static struct i2c_device_id xxxxxx_id_table[] = {
    {"xxxxxx",0},
    {/* sentinel */},
};

struct xxxxxx_device{
    struct i2c_client *client;
    struct regmap *regmap;
    struct regmap_config config;
    struct mutex lock;
};

#define xxxxxx_CHANEL(_TYPE){ \
        .type = _TYPE,               \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW) | BIT(IIO_CHAN_INFO_SCALE) ,    \
}

static const struct iio_chan_spec i2c_channels[] = {
    
    xxxxxx_CHANEL(IIO_INTENSITY),
    xxxxxx_CHANEL(IIO_PROXIMITY),
    xxxxxx_CHANEL(IIO_LIGHT),
};

static int xxxxxx_read_chanel_scale(struct iio_dev *indio_dev,const struct iio_chan_spec *i2c_channels,int *val2)
{ 
    return 0;
}
static int xxxxxx_read_chanel_raw(struct iio_dev *indio_dev,const struct iio_chan_spec *i2c_channels,int *val)
{ 
    return 0;
}

static int xxxxxx_read_raw(struct iio_dev *indio_dev,struct iio_chan_spec const *chan,int *val,int *val2,long mask)
{
    return 0;
}

static int xxxxxx_write_raw_get_fmt(struct iio_dev *indio_dev,struct iio_chan_spec const *chan,long mask)
{
    return 0;
}

static int xxxxxx_write_chanel_scale(struct iio_dev *indio_dev,const struct iio_chan_spec *i2c_channels,int val2)
{ 
    return 0;
}
static int xxxxxx_write_raw(struct iio_dev *indio_dev,struct iio_chan_spec const *chan,int val,int val2,long mask)
{  
    return 0;
}

static const struct iio_info xxxxxx_info = {
    .read_raw = xxxxxx_read_raw,
    .write_raw = xxxxxx_write_raw,
    .write_raw_get_fmt = xxxxxx_write_raw_get_fmt,
};



static int xxxxxx_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    struct xxxxxx_device *pri;
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
    indio_dev->info = &xxxxxx_info;

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

    
    return 0;
}

static int xxxxxx_remove(struct i2c_client *client)
{
    struct iio_dev *indio_dev = i2c_get_clientdata(client);
    struct xxxxxx_device *pri;

    pri = iio_priv(indio_dev);

    regmap_exit(pri->regmap);
    iio_device_unregister(indio_dev);

    return 0;
}

static struct i2c_driver xxxxxx_driver = {
    .probe = xxxxxx_probe,
    .remove = xxxxxx_remove,
    .driver = {
        .name = "xxxxxx",
        .owner = THIS_MODULE,
        .of_match_table = xxxxxx_match_table,
    },
    .id_table = xxxxxx_id_table,   /*no device tree*/
};

static int __init xxxxxx_init(void)
{
    return i2c_add_driver(&xxxxxx_driver);
}

static void __exit xxxxxx_exit(void)
{
    i2c_del_driver(&xxxxxx_driver);
}

module_init(xxxxxx_init);
module_exit(xxxxxx_exit);
MODULE_LICENSE("GPL");

