#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define PRIORITY 6

#define MSEC_H (60 * 60 * 1000)
#define MSEC_M (60 * 1000)
#define MSEC_S 1000

void entry_point(void *x, void *y, void *z)
{
    while (1) {
        int64_t time = k_uptime_get();
        int64_t h = time / MSEC_H;
        time %= MSEC_H;
        int64_t m = time / MSEC_M;
        time %= MSEC_M;
        int64_t s = time / MSEC_S;
        time %= MSEC_S;
        printk("%lld:%lld:%lld:%lld\n", h, m, s, time);
        k_msleep(1000);
    }
}

K_THREAD_DEFINE(snd_thread, STACK_SIZE, entry_point, NULL, NULL, NULL, PRIORITY, 0, 0);

int main()
{
    while (1)
    {
        k_sleep(K_SECONDS(5));
    }
}