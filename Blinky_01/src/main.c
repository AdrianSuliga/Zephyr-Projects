#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

static int configure_leds(void);
static int devices_ready(void);

static const struct gpio_dt_spec embedded_led = GPIO_DT_SPEC_GET(DT_NODELABEL(embedded_led), gpios);
static const struct gpio_dt_spec ext_led1 = GPIO_DT_SPEC_GET(DT_NODELABEL(ext_led1), gpios);
static const struct gpio_dt_spec ext_led2 = GPIO_DT_SPEC_GET(DT_NODELABEL(ext_led2), gpios);
static const struct gpio_dt_spec ext_led3 = GPIO_DT_SPEC_GET(DT_NODELABEL(ext_led3), gpios);

int main()
{
    int ret = 0;
    
    if (!devices_ready()) {
        return -1;
    }

    if (!configure_leds()) {
        return -1;
    }

    for (int i = 0; i < 10; i++) {
        ret = gpio_pin_toggle_dt(&embedded_led);
        if (ret != 0) {
            return -1;
        }
        k_msleep(100);
    }

    while (true) {
        ret = gpio_pin_toggle_dt(&ext_led1);
        if (ret != 0) {
            return -1;
        }

        k_msleep(500);

        ret = gpio_pin_toggle_dt(&ext_led2);
        if (ret != 0) {
            return -1;
        }

        k_msleep(500);

        ret = gpio_pin_toggle_dt(&ext_led3);
        if (ret != 0) {
            return -1;
        }

        k_msleep(500);
    }

    return 0;
}

static int configure_leds(void)
{
    int ret = gpio_pin_configure_dt(&ext_led1, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&ext_led2, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return 0;
    }
    
    ret = gpio_pin_configure_dt(&ext_led3, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return 0;
    }

    ret = gpio_pin_configure_dt(&embedded_led, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return 0;
    }

    return 1;
}

static int devices_ready(void) {
    if (!device_is_ready(embedded_led.port)) {
        return 0;
    }

    if (!device_is_ready(ext_led1.port)) {
        return 0;
    }

    if (!device_is_ready(ext_led2.port)) {
        return 0;
    }

    if (!device_is_ready(ext_led3.port)) {
        return 0;
    }
    
    return 1;
}