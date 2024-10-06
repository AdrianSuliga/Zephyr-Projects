#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>

#include <string.h>

#define WIFI_CONNECT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)
#define WIFI_SCAN_EVENTS (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE)

#define WIFI_EVENTS (WIFI_CONNECT_EVENTS | WIFI_SCAN_EVENTS)

struct net_mgmt_event_callback wifi_callback;

bool is_connected = false; 
bool ryszard_found = false;

void wifi_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface)
{
    switch (event)
    {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        printk("Connection attempt finished\n");
        struct wifi_status *received_status = (struct wifi_status*)(cb->info);
        if (received_status->conn_status == WIFI_STATUS_CONN_SUCCESS) {
            is_connected = true;
            printk("Connected\n");
        } else {
            is_connected = false;
            printk("Not connected, error: %d\n", received_status->conn_status);
        }
        break;

    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        is_connected = false;
        printk("Disconnected, error: %d\n", *((int32_t*)(cb->info)));
        break;

    case NET_EVENT_WIFI_SCAN_RESULT:
        uint8_t* data = (uint8_t*)(cb->info);

        if (!strncmp(data, "Ryszard", sizeof("Ryszard"))) {
            printk("Found Ryszard\n");
            ryszard_found = true;
        }
        break;

    case NET_EVENT_WIFI_SCAN_DONE:
        printk("Scanning finished ");
        if (!ryszard_found) {
            printk("Ryszard not found");
        }
        printk("\n");
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
        .ssid = "Ryszard",
        .ssid_length = sizeof("Ryszard") - 1,
        .psk = "jestnatrello",
        .psk_length = sizeof("jestnatrello") - 1,
        .band = 0,
        .security = WIFI_SECURITY_TYPE_PSK,
    };

    net_mgmt_init_event_callback(&wifi_callback, wifi_handler, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_callback);

    net_mgmt(NET_REQUEST_WIFI_SCAN, network_interface, NULL, 0);

    if (ryszard_found) {
        net_mgmt(NET_REQUEST_WIFI_CONNECT, network_interface, &wifi_params, sizeof(wifi_params));
    }

    while (1)
    {
        if (!ryszard_found) {
            net_mgmt(NET_REQUEST_WIFI_SCAN, network_interface, NULL, 0);
        }

        if (ryszard_found && !is_connected) {
            net_mgmt(NET_REQUEST_WIFI_CONNECT, network_interface, &wifi_params, sizeof(wifi_params));
        }

        printk("Main running...\n");
        k_sleep(K_SECONDS(2));
    }
    
    return 0;
}