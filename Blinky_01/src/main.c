#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

static int configure_leds(void);

#define HIGH_STATE 1
#define LOW_STATE 0

#define LED_EMB 2
#define LED1_PIN 4
#define LED2_PIN 5
#define LED3_PIN 18

static const struct device *gpio_device = DEVICE_DT_GET(DT_NODELABEL(gpio0));

int main()
{
    int ret = 0;

    if (!device_is_ready(gpio_device)) {
        return -1;
    }

    ret = configure_leds();

    if (ret != 0) {
        return -1;
    }

    for (int i = 0; i < 10; i++) {
        ret = gpio_pin_toggle(gpio_device, LED_EMB);
        if (ret != 0) {
            return -1;
        }
        k_msleep(100);
    }

    while (true) {
        ret = gpio_pin_toggle(gpio_device, LED1_PIN);
        if (ret != 0) {
            return -1;
        }

        k_msleep(500);

        ret = gpio_pin_toggle(gpio_device, LED2_PIN);
        if (ret != 0) {
            return -1;
        }

        k_msleep(500);

        ret = gpio_pin_toggle(gpio_device, LED3_PIN);
        if (ret != 0) {
            return -1;
        }

        k_msleep(500);
    }

}

static int configure_leds(void)
{
    int ret = gpio_pin_configure(gpio_device, LED1_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return ret;
    }

    ret = gpio_pin_configure(gpio_device, LED2_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return ret;
    }
    
    ret = gpio_pin_configure(gpio_device, LED3_PIN, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return ret;
    }

    ret = gpio_pin_configure(gpio_device, LED_EMB, GPIO_OUTPUT_ACTIVE);
    if (ret != 0) {
        return ret;
    }

    return 0;
}