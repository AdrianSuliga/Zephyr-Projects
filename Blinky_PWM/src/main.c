#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>

static const struct pwm_dt_spec fading_led = PWM_DT_SPEC_GET(DT_NODELABEL(fading_led));

int main(void)
{
    if (!device_is_ready(fading_led.dev)) {
        return -1;
    }

    int ret = 0;
    int pulse = 0;

    while (1) {
        while (pulse < 10000) {
            pwm_set_pulse_dt(&fading_led, pulse);
            pulse += 10;
            k_msleep(1);
        }
        while (pulse > 0) {
            pwm_set_pulse_dt(&fading_led, pulse);
            pulse -= 10;
            k_msleep(1);
        }
    }
    
    return 0;
}