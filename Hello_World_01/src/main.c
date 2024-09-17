#include <zephyr/kernel.h>

int main()
{
    while (1)
    {
        printk("Hello world\n");
        k_msleep(1000);
    }
    return 0;    
}
