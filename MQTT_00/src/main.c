#include <zephyr/kernel.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>

#include <zephyr/logging/log.h>

#include <string.h>

#define WIFI_SCAN_EVENTS (NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE)
#define WIFI_CONNECT_EVENTS (NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT)
#define WIFI_EVENTS (WIFI_SCAN_EVENTS | WIFI_CONNECT_EVENTS)

#define WIFI_SSID "Stopki Nahidy 604"
#define WIFI_PSK "GrupaWielbicieliGenshina2137"

#define MQTT_CLIENT "zephyr_esp_mqtt_client"
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883
#define MQTT_PUB_TOPIC "zephyr/wifi/test/publish"

#define MQTT_MESSAGE_BUFFER_SIZE 256
#define MQTT_PAYLOAD_BUFFER_SIZE 256

static uint8_t rx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_MESSAGE_BUFFER_SIZE];

static struct net_mgmt_event_callback wifi_callback;

static struct sockaddr_in broker;
static struct mqtt_client client;

bool connected_to_wifi = false;

static void fill_wifi_connect_params(struct wifi_connect_req_params *wifi_params);
void wifi_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface);

void mqtt_handler(struct mqtt_client *c, const struct mqtt_evt *evt);
static int client_init(struct mqtt_client *client);
static int server_resolve(void);
static int publish_message(struct mqtt_client *c, enum mqtt_qos qos, uint8_t* data, size_t len);
int mqtt_connect_publish(struct mqtt_client *client, uint8_t *data, size_t len);

LOG_MODULE_REGISTER(Main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("ESP32 booted");

    struct net_if* network_interface = net_if_get_default();
    static struct wifi_connect_req_params wifi_params;
    uint8_t data_to_send = 65;

    fill_wifi_connect_params(&wifi_params);

    net_mgmt_init_event_callback(&wifi_callback, wifi_handler, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_callback);

    k_sleep(K_SECONDS(5));

    net_mgmt(NET_REQUEST_WIFI_CONNECT, network_interface, &wifi_params, sizeof(wifi_params));

    while (1) {
        if (connected_to_wifi) {
            int err = mqtt_connect_publish(&client, &data_to_send, sizeof(data_to_send));
            if (err != 0) {
                LOG_ERR("Failed to disconnect with error %d", err);
                LOG_INF("Retrying in 1s...");
                k_sleep(K_SECONDS(1));
            } else {
                LOG_INF("INCREMENTING");
                if (data_to_send == 91) {
                    data_to_send -= 28;
                }
                data_to_send++;
            }
        } else {
            net_mgmt(NET_REQUEST_WIFI_CONNECT, network_interface, &wifi_params, sizeof(wifi_params));
        }
        k_sleep(K_SECONDS(1));
    }

    return 0;
}

void fill_wifi_connect_params(struct wifi_connect_req_params *wifi_params) 
{
    wifi_params->ssid = WIFI_SSID;
    wifi_params->ssid_length = strlen(WIFI_SSID);
    wifi_params->psk = WIFI_PSK;
    wifi_params->psk_length = strlen(WIFI_PSK);
    wifi_params->band = 0;
    wifi_params->security = WIFI_SECURITY_TYPE_PSK;
}

void wifi_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface)
{
    switch (event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        struct wifi_status *received_status = (struct wifi_status *)(cb->info);
        if (received_status->conn_status == WIFI_STATUS_CONN_SUCCESS) {
            connected_to_wifi = true;
            LOG_INF("Connected");
        } else {
            connected_to_wifi = false;
            LOG_INF("Not connected, error: %d", received_status->conn_status);
        }
        break;

    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        connected_to_wifi = false;
        LOG_INF("Disconnected, error: %d", *((int32_t *)(cb->info)));
        break;

    case NET_EVENT_WIFI_SCAN_RESULT:
        uint8_t *data = (uint8_t *)(cb->info);

        if (!strncmp(data, WIFI_SSID, sizeof(WIFI_SSID))) {
            LOG_INF("Found %s", WIFI_SSID);
        }
        break;

    case NET_EVENT_WIFI_SCAN_DONE:
        LOG_INF("Scanning finished ");
        break;

    default:
        LOG_INF("Invalid Net Event!\n");
        break;
    }
}

int mqtt_connect_publish(struct mqtt_client *client, uint8_t *data, size_t len)
{
    int err = 0;
    struct pollfd fds;

    err = client_init(client);
    if (err != 0) {
        LOG_ERR("Client not initialized");
        return err;
    } 

    err = mqtt_connect(client);
    if (err != 0) {
        LOG_ERR("Unable to connect to broker, %d", err);
        return mqtt_disconnect(client);    
    } 

    fds.fd = client->transport.tcp.sock;
    fds.events = POLLIN;

    err = poll(&fds, 1, mqtt_keepalive_time_left(client));
    if (err < 0) {
        LOG_ERR("Error in poll %d", err);
        return mqtt_disconnect(client);
    }

    err = mqtt_live(client);
    if (err != 0 && err != -EAGAIN) {
        LOG_ERR("Error in mqtt_live %d", err);
        return mqtt_disconnect(client);
    }

    if ((fds.revents & POLLIN) == POLLIN) {
        err = mqtt_input(client);
        if (err != 0) {
            LOG_ERR("Error in mqtt_input %d", err);
            return mqtt_disconnect(client);
        }
    }

    if ((fds.revents & POLLERR) == POLLERR) {
	    LOG_ERR("POLLERR");
        return mqtt_disconnect(client);
    }

    if ((fds.revents & POLLNVAL) == POLLNVAL) {
	    LOG_ERR("POLLNVAL");
        return mqtt_disconnect(client);
    }

    err = mqtt_live(client);
    if (err != 0 && err != -EAGAIN) {
        LOG_ERR("mqtt_live error %d", err);
        return mqtt_disconnect(client);
    }

    err = publish_message(client, MQTT_QOS_1_AT_LEAST_ONCE, data, len);
    if (err != 0) {
        LOG_ERR("Publishing failed, %d", err);
        return mqtt_disconnect(client);
    }

    err = mqtt_disconnect(client);
    if (err != 0) {
        LOG_ERR("Error in mqtt_disconnect %d", err);
        return err;
    }

    return err;
}

void mqtt_handler(struct mqtt_client *c, const struct mqtt_evt *evt)
{
    switch (evt->type) {
        case MQTT_EVT_CONNACK:
            if (evt->result != 0) {
                LOG_ERR("Unable to connect %d", evt->result);
                break;
            }
            LOG_INF("MQTT connected");
            break;
        case MQTT_EVT_DISCONNECT:
            LOG_INF("MQTT disconnected");
            break;
        case MQTT_EVT_PUBLISH:
            LOG_INF("Received message on topic %s", evt->param.publish.message.topic.topic.utf8);
            break;
        default:
            LOG_INF("Unhandled mqtt event");
            break;
    }
}

int client_init(struct mqtt_client *client)
{
    int err;
    mqtt_client_init(client);

    err = server_resolve();
    if (err != 0) {
        LOG_ERR("Failed to initiate broker connection");
        return err;
    }

    client->broker = &broker;
    client->evt_cb = mqtt_handler;
    client->client_id.utf8 = MQTT_CLIENT;
    client->client_id.size = strlen(MQTT_CLIENT);
    client->password = NULL;
    client->user_name = NULL;
    client->protocol_version = MQTT_VERSION_3_1_1;

    client->rx_buf = rx_buffer;
    client->rx_buf_size = sizeof(rx_buffer);
    client->tx_buf = tx_buffer;
    client->tx_buf_size = sizeof(tx_buffer);

    client->transport.type = MQTT_TRANSPORT_NON_SECURE;

    return err;
}

static int server_resolve(void)
{
    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM
    };

    err = getaddrinfo(MQTT_BROKER, NULL, &hints, &result);
    while (err) {
        LOG_ERR("get addr info failed %d %s", err, gai_strerror(err));
        err = getaddrinfo(MQTT_BROKER, NULL, &hints, &result);
        k_sleep(K_SECONDS(1));
    }

    if (result == NULL) {
        LOG_ERR("address not found");
        return -ENOENT;
    }

    broker.sin_family = AF_INET;
    broker.sin_port = htons(MQTT_BROKER_PORT);
    broker.sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;

    freeaddrinfo(result);

    return err;
}

int publish_message(struct mqtt_client *c, enum mqtt_qos qos, uint8_t* data, size_t len) 
{
    struct mqtt_publish_param params;
    
    params.message.topic.qos = qos;
    params.message.topic.topic.utf8 = MQTT_PUB_TOPIC;
    params.message.topic.topic.size = strlen(MQTT_PUB_TOPIC);
    params.message.payload.data = data;
    params.message.payload.len = len;
    params.message_id = 2137;
    params.dup_flag = 0;
    params.retain_flag = 0;

    LOG_INF("Publishing to topic %s", MQTT_PUB_TOPIC);
    return mqtt_publish(c, &params);
}