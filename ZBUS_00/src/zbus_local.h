#include <zephyr/zbus/zbus.h>

struct test_msg {
    int x;
    int y;
    int z;
};

ZBUS_CHAN_DECLARE(test_channel);