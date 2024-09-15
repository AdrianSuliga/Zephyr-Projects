#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define HIGH_STATE 1
#define LOW_STATE 0

#define LED_PIN 4

static const struct device *gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

int main()
{
    if (!device_is_ready(gpio_dev)) {
        return -1;
    }

    int ret;

    ret = gpio_pin_configure(gpio_dev, LED_PIN, GPIO_OUTPUT_ACTIVE);

    if (ret != 0) {
        return -1;
    }

    while (1) {
        ret = gpio_pin_set_raw(gpio_dev, LED_PIN, HIGH_STATE);
        if (ret != 0) {
            return -1;
        }
        k_msleep(1000);

        ret = gpio_pin_set_raw(gpio_dev, LED_PIN, LOW_STATE);
        if (ret != 0) {
            return -1;
        }
        k_msleep(1000);
    }

    return 0;
}