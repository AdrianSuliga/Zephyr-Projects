/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/wifi.h>

#define WIFI_SCAN_EVT (NET_REQUEST_WIFI_SCAN | NET_EVENT_WIFI_SCAN_RESULT | NET_EVENT_WIFI_SCAN_DONE)

#define SSID_LENGTH_ADDR 32
#define BAND_ADDR 33
#define CHANNEL_ADDR 34
#define SECURITY_TYPE_ADDR 36
#define RSSI_ADDR 44
#define MAC_LENGTH_ADDR 51

#define SSID_START 0
#define MAC_START 45

struct net_mgmt_event_callback wifi_callback;
struct wifi_scan_result result;

static void print_wifi_scan_result(struct wifi_scan_result *result)
{
	printk("SSID: %s", (char*)result->ssid);
	for (uint8_t i = result->ssid_length; i < WIFI_SSID_MAX_LEN; i++) {
		printk(" ");
		k_msleep(10);
	}
	printk("  ");

	if (result->band) {
		printk("BAND: 5 GHz  ");
	} else {
		printk("BAND: 2.4 GHz  ");
	}

	printk("CHANNEL: %d  ", result->channel);
	printk("SEC: %u  ", result->security);
	printk("RSSI: %d  ", result->rssi);
	printk("BSSID: %X:%X:%X:%X:%X:%X  ", result->mac[0], result->mac[1], result->mac[2], result->mac[3], result->mac[4], result->mac[5]);
	printk("MAC LEN:  %d\n", result->mac_length);
}

void callback_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	switch (mgmt_event)
	{
	case NET_EVENT_WIFI_SCAN_RESULT:
		uint8_t* cbinfo = (uint8_t*)(cb->info);
		
		result.ssid_length = cbinfo[SSID_LENGTH_ADDR];
		result.band = cbinfo[BAND_ADDR];
		result.channel = cbinfo[CHANNEL_ADDR];
		result.security = cbinfo[SECURITY_TYPE_ADDR];
		result.rssi = cbinfo[RSSI_ADDR];

		for (uint8_t i = SSID_START; i < SSID_START + WIFI_SSID_MAX_LEN; i++) {
			result.ssid[i] = cbinfo[i];
		}

		for (uint8_t i = MAC_START; i < MAC_START + WIFI_MAC_ADDR_LEN; i++) {
			result.mac[i - MAC_START] = cbinfo[i]; 
		}

		result.mac_length = cbinfo[MAC_LENGTH_ADDR];

		print_wifi_scan_result(&result);

		k_msleep(100);
		printk("\n");
		break;

	case NET_EVENT_WIFI_SCAN_DONE:
		printk("Finished scanning\n");
		break;

	case NET_REQUEST_WIFI_SCAN:
		printk("Scan requested...\n");
		break;

	default:
		break;
	}
}

int main(void)
{
	struct net_if *iface;
	iface = net_if_get_default();
	
	net_mgmt_init_event_callback(&wifi_callback, callback_handler, WIFI_SCAN_EVT);
	net_mgmt_add_event_callback(&wifi_callback);

	net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);

	while (1) {
		k_sleep(K_SECONDS(5));
	}

	return 0;
}
