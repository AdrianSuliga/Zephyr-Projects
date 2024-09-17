#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>

static struct gpio_callback button_cb_data;
static const struct gpio_dt_spec ext_led = GPIO_DT_SPEC_GET(DT_NODELABEL(ext_led), gpios);
static const struct gpio_dt_spec pot_button = GPIO_DT_SPEC_GET(DT_NODELABEL(pot_button), gpios);

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pin)
{
    if (gpio_pin_toggle_dt(&ext_led)) {
        printk("COULD NOT TOGGLE LED \n");
    }
}

int main()
{
    if (!device_is_ready(ext_led.port)) 
        return -1;
    if (!device_is_ready(pot_button.port))
        return -1;

    if (gpio_pin_configure_dt(&ext_led, GPIO_OUTPUT_INACTIVE))
        return -1;
    if (gpio_pin_configure_dt(&pot_button, GPIO_INPUT))
        return -1;

    if (gpio_pin_interrupt_configure_dt(&pot_button, GPIO_INT_EDGE_TO_ACTIVE))
        return -1;
    gpio_init_callback(&button_cb_data, button_pressed, BIT(pot_button.pin));
    if (gpio_add_callback(pot_button.port, &button_cb_data))
        return -1;

    while (1) {

    }

    return 0;
}