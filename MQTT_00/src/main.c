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

#define WIFI_SSID "Ryszard"
#define WIFI_PSK "jestnatrello"

#define MQTT_CLIENT "zephyr_esp_mqtt_client"
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883
#define MQTT_PUB_TOPIC "zephyr/wifi/test/publish"
#define MQTT_SUB_TOPIC "zephyr/wifi/test/subscribe"

#define MQTT_MESSAGE_BUFFER_SIZE 256
#define MQTT_PAYLOAD_BUFFER_SIZE 256

static uint8_t rx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t tx_buffer[MQTT_MESSAGE_BUFFER_SIZE];
static uint8_t payload_buffer[MQTT_PAYLOAD_BUFFER_SIZE];

static struct net_mgmt_event_callback wifi_callback;

struct wifi_connect_req_params wifi_params;

static struct sockaddr_in broker;
static struct mqtt_client client;

bool connected_to_wifi = false;
bool mqtt_connected = false;

static void fill_wifi_connect_params(struct wifi_connect_req_params *wifi_params);

void wifi_handler(struct net_mgmt_event_callback *cb, uint32_t event, struct net_if *iface);
void mqtt_handler(struct mqtt_client *c, const struct mqtt_evt *evt);

void init_wifi_connection(struct net_if *interface);
void init_mqtt_connection(struct pollfd *fds);

static int client_init(struct mqtt_client *client);
static int server_resolve(void);
static int publish_message(struct mqtt_client *c, enum mqtt_qos qos, uint8_t *data, size_t len);
static int subscribe_topic(struct mqtt_client *c);
static int get_payload(struct mqtt_client *c, size_t len);

LOG_MODULE_REGISTER(Main, LOG_LEVEL_INF);

int main(void)
{
    LOG_INF("ESP32 booted");

    struct net_if *network_interface = net_if_get_default();
    struct pollfd fds; 
    uint8_t data_to_send = 65;

    fill_wifi_connect_params(&wifi_params);

    net_mgmt_init_event_callback(&wifi_callback, wifi_handler, WIFI_EVENTS);
    net_mgmt_add_event_callback(&wifi_callback);

    k_sleep(K_SECONDS(1));

    init_wifi_connection(network_interface);
    init_mqtt_connection(&fds);

    while (1) {
        if (!mqtt_connected) {
            LOG_INF("Retrying MQTT connection");
            init_mqtt_connection(&fds);
        }

        int err = poll(&fds, 1, mqtt_keepalive_time_left(&client));
        if (err < 0) {
            LOG_ERR("Error in poll %d", err);
            k_sleep(K_SECONDS(1));
            continue;
        }

        err = mqtt_live(&client);
        if ((err != 0) && (err != -EAGAIN)) {
            LOG_ERR("Error in mqtt_live %d", err);
            k_sleep(K_SECONDS(1));
            continue;
        }

        if ((fds.revents & POLLIN) == POLLIN) {
            err = mqtt_input(&client);
            if (err != 0) {
                LOG_ERR("Error in mqtt_input %d", err);
                k_sleep(K_SECONDS(1));
                continue;
            }
        }

		if ((fds.revents & POLLERR) == POLLERR) {
			LOG_ERR("POLLERR");
            k_sleep(K_SECONDS(1));
			continue;
		}

		if ((fds.revents & POLLNVAL) == POLLNVAL) {
			LOG_ERR("POLLNVAL");
            k_sleep(K_SECONDS(1));
			continue;
		}

        err = publish_message(&client, MQTT_QOS_1_AT_LEAST_ONCE, &data_to_send, sizeof(data_to_send));
        if (err != 0) {
            LOG_ERR("Error in publish, %d", err);
            k_sleep(K_SECONDS(1));
            continue;
        }

        k_sleep(K_SECONDS(1));
    }

    return 0;
}

static void fill_wifi_connect_params(struct wifi_connect_req_params *wifi_params)
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
    switch (event)
    {
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
        LOG_INF("Scanning finished");
        break;

    default:
        LOG_INF("Invalid Net Event!");
        break;
    }
}

void mqtt_handler(struct mqtt_client *c, const struct mqtt_evt *evt)
{
    switch (evt->type)
    {
    case MQTT_EVT_CONNACK:
        if (evt->result != 0) {
            mqtt_connected = false;
            LOG_ERR("Unable to connect %d", evt->result);
            break;
        }
        mqtt_connected = true;
        LOG_INF("MQTT connected");
        subscribe_topic(&client);
        break;
    case MQTT_EVT_DISCONNECT:
        mqtt_connected = false;
        LOG_INF("MQTT disconnected");
        break;
    case MQTT_EVT_PUBLISH:
        LOG_INF("Received message on topic %s", evt->param.publish.message.topic.topic.utf8);
        const struct mqtt_publish_param *p = &evt->param.publish;
        int err = get_payload(c, p->message.payload.len);

        if (p->message.topic.qos == MQTT_QOS_1_AT_LEAST_ONCE) {
            const struct mqtt_puback_param ack = {.message_id = p->message_id};
            mqtt_publish_qos1_ack(c, &ack);
        }

        if (err >= 0) {
            printk("Received: %s\n", payload_buffer);
        } else if (err == -EMSGSIZE) {
            LOG_ERR("Received %d bytes, buffer has %d bytes", p->message.payload.len, MQTT_PAYLOAD_BUFFER_SIZE);
        } else {
            LOG_ERR("get_payload failed %d", err);

            err = mqtt_disconnect(c);
            if (err) {
                LOG_ERR("Failed to disconnect %d", err);
            }
        }

        break;
    case MQTT_EVT_PUBACK:
        break;
    default:
        LOG_INF("Unhandled mqtt event, %d", evt->type);    
        break;
    }
}

static int client_init(struct mqtt_client *client)
{
    int err;
    mqtt_client_init(client);

    err = server_resolve();
    if (err != 0)
    {
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
        .ai_socktype = SOCK_STREAM};

    err = getaddrinfo(MQTT_BROKER, NULL, &hints, &result);
    while (err)
    {
        LOG_ERR("get addr info failed %d %s", err, gai_strerror(err));
        err = getaddrinfo(MQTT_BROKER, NULL, &hints, &result);
        k_sleep(K_SECONDS(1));
    }

    if (result == NULL)
    {
        LOG_ERR("address not found");
        return -ENOENT;
    }

    broker.sin_family = AF_INET;
    broker.sin_port = htons(MQTT_BROKER_PORT);
    broker.sin_addr.s_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;

    freeaddrinfo(result);

    return err;
}

static int publish_message(struct mqtt_client *c, enum mqtt_qos qos, uint8_t *data, size_t len)
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

static int subscribe_topic(struct mqtt_client *c)
{
    struct mqtt_topic subscribe_topic = {
        .topic = {
            .utf8 = MQTT_SUB_TOPIC,
            .size = strlen(MQTT_SUB_TOPIC)
        },
        .qos = MQTT_QOS_1_AT_LEAST_ONCE
    };

    const struct mqtt_subscription_list sub_list = {
        .list = &subscribe_topic,
        .list_count = 1,
        .message_id = 4269
    };

    LOG_INF("Subscribing to %s", MQTT_SUB_TOPIC);
    return mqtt_subscribe(c, &sub_list);
}

void init_wifi_connection(struct net_if *interface)
{
    while (!connected_to_wifi) {
        LOG_INF("Trying to connect...");
        net_mgmt(NET_REQUEST_WIFI_CONNECT, interface, &wifi_params, sizeof(wifi_params));
        k_sleep(K_SECONDS(2));
    }
}

void init_mqtt_connection(struct pollfd *fds)
{
    int err = -1;
    while (err) {
        err = client_init(&client);
        if (err != 0) {
            LOG_ERR("Client not initialized");
            continue;
        }

        err = mqtt_connect(&client);
        if (err != 0) {
            LOG_ERR("Unable to connect to broker, %d", err);
            continue;
        }

        fds->fd = client.transport.tcp.sock;
        fds->events = POLLIN;
    }
}

static int get_payload(struct mqtt_client *c, size_t len)
{
    int err = 0;
    if (len > MQTT_PAYLOAD_BUFFER_SIZE) {
        return -EMSGSIZE;
    }

    err = mqtt_readall_publish_payload(c, payload_buffer, len);
    if (err) {
        return err;
    }

    return err;
}