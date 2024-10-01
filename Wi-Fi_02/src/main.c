#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>

struct net_mgmt_event_callback wifi_connect_callback;

bool is_connected = false; 

void callback_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface)
{
    switch (event)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        printk("Connection result received\n");
        struct wifi_status *received_status = (struct wifi_status*)(cb->info);
        if (received_status->conn_status == WIFI_STATUS_CONN_SUCCESS) {
            is_connected = true;
            printk("Connected\n");
        } else {
            is_connected = false;
            printk("Not connected, error: %d\n", received_status->conn_status);
        }
        break;

    default:
        printk("Invalid Net Event!\n");
        break;
    }
}

int main() 
{    
    struct net_if *network_interface;
    network_interface = net_if_get_default();

    struct wifi_connect_req_params wifi_params = {
        .ssid = "Byszard",
        .ssid_length = sizeof("Byszard") - 1,
        .psk = "jestnatrello",
        .psk_length = sizeof("jestnatrello") - 1,
        .band = 0,
        .security = WIFI_SECURITY_TYPE_PSK,
    };

    net_mgmt_init_event_callback(&wifi_connect_callback, callback_handler, NET_EVENT_WIFI_CONNECT_RESULT);
    net_mgmt_add_event_callback(&wifi_connect_callback);

    net_mgmt(NET_REQUEST_WIFI_CONNECT, network_interface, &wifi_params, sizeof(wifi_params));

    while (1)
    {
        if (!is_connected) {
            printk("Retrying connection...\n");
            net_mgmt(NET_REQUEST_WIFI_CONNECT, network_interface, &wifi_params, sizeof(wifi_params));
        }

        printk("Main running...\n");
        k_sleep(K_SECONDS(5));
    }
    
    return 0;
}