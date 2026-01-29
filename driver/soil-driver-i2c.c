#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

#define DRIVER_NAME "soil-driver-i2c"

static int soil_sensor_probe(struct i2c_client *client)
{
    dev_info(&client->dev, "Probing soil sensor at 0x%02x\n", client->addr);
    return 0;
}

static void soil_sensor_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "Removing soil sensor driver\n");
}

static const struct of_device_id soil_sensor_of_match[] = {
    { .compatible = "christian,soil-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, soil_sensor_of_match);

static struct i2c_driver soil_sensor_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = soil_sensor_of_match,
    },
    .probe = soil_sensor_probe,
    .remove = soil_sensor_remove,
};

module_i2c_driver(soil_sensor_driver);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Christian Bull");
MODULE_DESCRIPTION("A simple I2C driver for a soil humidity sensor");