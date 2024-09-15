#include <zephyr/kernel.h>
void main(void)
{
    while (1) {
        k_msleep(1000);
    }
}