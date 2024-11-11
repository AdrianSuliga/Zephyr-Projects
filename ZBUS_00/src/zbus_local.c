#include "zbus_local.h"

ZBUS_CHAN_DEFINE(test_channel,
                struct test_msg,
                NULL,
                NULL,
                ZBUS_OBSERVERS(my_listener),
                ZBUS_MSG_INIT(.x = 0, .y = 0, .z = 0)
);

