#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include "zbus_local.h"

void listener_callback(const struct zbus_channel *chan) 
{
    const struct test_msg *msg;
    if (&test_channel == chan) {
        msg = zbus_chan_const_msg(chan);
        printk("From listener -> %d %d %d\n", msg->x, msg->y, msg->z);
    }
}

ZBUS_LISTENER_DEFINE(my_listener, listener_callback);

void thread_entry1(void) 
{
    while (1) {
        printk("Thread 1 running...\n");
        k_sleep(K_SECONDS(1));
    }
}
K_THREAD_DEFINE(thread1, 1024, thread_entry1, NULL, NULL, NULL, 3, 0, 0);