#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include "zbus_local.h"

void thread_entry2(void)
{
    while (1) {
        printk("Thread 2 running...\n");
        printk("Publishing to channel...\n");
        const struct test_msg msg = {
            .x = 2,
            .y = 1,
            .z = 3
        };
        zbus_chan_pub(&test_channel, &msg, K_FOREVER);
        k_sleep(K_SECONDS(1));
    }
}
K_THREAD_DEFINE(thread2, 1024, thread_entry2, NULL, NULL, NULL, 3, 0, 0);