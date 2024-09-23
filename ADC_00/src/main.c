#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>

int16_t buffer1;

static const struct adc_dt_spec adc_spec = ADC_DT_SPEC_GET_BY_IDX(DT_PATH(zephyr_user), 0);
static struct adc_sequence adc_res;

int main(void)
{    
    if (!device_is_ready(adc_spec.dev)) {
        return -1;
    }

    if (adc_sequence_init_dt(&adc_spec, &adc_res) != 0) {
        printk("SEQ ERR\n");
        return -1;
    }

    adc_res.buffer = &buffer1;
    adc_res.buffer_size = sizeof(buffer1);

    while (true) {
        if (adc_channel_setup_dt(&adc_spec) != 0) {
            printk("CHANNEL_ERR\n");
            return -1;
        }

        adc_read_dt(&adc_spec, &adc_res);

        adc_res.buffer = &buffer1;
        printk("ADC = %d\n", buffer1);

        k_msleep(2000);
    }

    return 0;
}