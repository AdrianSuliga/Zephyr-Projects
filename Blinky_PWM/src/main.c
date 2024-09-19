#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

#define SLEEPTIME 25

static uint32_t ret = 0;
static uint32_t pulse = 0;
static bool is_increasing = true;

static const struct pwm_dt_spec fading_led = PWM_DT_SPEC_GET(DT_NODELABEL(fading_led));

static void delta_timer_handler(struct k_timer *timer_info)
{
    if (is_increasing) {
        ret = pwm_set_pulse_dt(&fading_led, pulse);
        pulse += 100;
        if (pulse == 7500)
            is_increasing = false;
    } else {
        ret = pwm_set_pulse_dt(&fading_led, pulse);
        pulse -= 100;
        if (pulse == 0)
            is_increasing = true;
    }

    if (ret != 0) {
        printk("PWM ERROR\n");
        return;
    }
}

K_TIMER_DEFINE(delta_timer, delta_timer_handler, NULL);

int main(void)
{
    if (!device_is_ready(fading_led.dev)) {
        return -1;
    }

    k_timer_start(&delta_timer, K_MSEC(SLEEPTIME), K_MSEC(SLEEPTIME));

    while (1) {

    }
    
    return 0;
}