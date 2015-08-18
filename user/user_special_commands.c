#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "driver/uart.h"
#include "user_uart.h"
#include "stdlib.h"
#include "osapi.h"
#include "user_debug.h"
#include "user_tcp.h"
#include "user_special_commands.h"
#include "user_wifi.h"
#include "utils/str.h"
#include "user_debug.h"
//----------------------------------------------------------------------------------
// This part is response for special command {debuging, keep alive} thru TCP
//----------------------------------------------------------------------------------


at_linkConType *netInfo = NULL;


uint8_t *authDesc[] = { "OPEN", "WEP", "WPA_PSK", "WPA2_PSK", "WPA_WPA2_PSK" };

/**
 * @brief  Wifi ap scan over callback to display.
 * @param  arg: contain the aps information
 * @param  status: scan over status
 * @retval None
 */
static void ICACHE_FLASH_ATTR
scan_done(void *arg, STATUS status) {
	uint8 ssid[33];
	char temp[128];

	if (status == OK) {
		struct bss_info *bss_link = (struct bss_info *) arg;
		bss_link = bss_link->next.stqe_next;			//ignore first

		while (bss_link != NULL ) {
			os_memset(ssid, 0, 33);
			if (os_strlen(bss_link->ssid) <= 32) {
				os_memcpy(ssid, bss_link->ssid, os_strlen(bss_link->ssid));
			} else {
				os_memcpy(ssid, bss_link->ssid, 32);
			}
			os_sprintf(temp, "{%s,\"%s\",%d dBm,\""MACSTR"\",%d}", authDesc[bss_link->authmode], ssid, bss_link->rssi, MAC2STR(bss_link->bssid),
					bss_link->channel);

			if (netInfo == NULL || netInfo->free) {
				netInfo = NULL;
				return;
			}

			my_espconn_sent_headered(netInfo, temp, strlen(temp), ASYNC_HEADER_WIFI_INFO, strlen(ASYNC_HEADER_WIFI_INFO));
			debug_print_str_tcp(temp);

			bss_link = bss_link->next.stqe_next;
		}

	} else {

	}
}

uint8_t ICACHE_FLASH_ATTR
special_cmd(tcp_data_to_exec_t *dte) {
	at_linkConType *link = (at_linkConType *) dte->link;
	//uint8_t *d, uint16_t l, at_linkConType *link
	uint8_t result = 1;

	if (strcmp(dte->content, DEBUG_CMD) == 0) {

		set_debug_msg_target(link);

	} else if (strcmp(dte->content, KEEP_ALIVE_CMD) == 0) {

		my_espconn_sent_headered(link, KEEP_ALIVE_CMD, strlen(KEEP_ALIVE_CMD), dte->header, strlen(dte->header));

	} else if (strcmp(dte->content, SCAN_NETWORK_CMD) == 0) {

		wifi_station_scan(NULL, scan_done);
		my_espconn_sent_headered(link, SCAN_NETWORK_CMD, strlen(SCAN_NETWORK_CMD), dte->header, strlen(dte->header));
		netInfo = link;

	} else if (strcmp(dte->content, MAC_INFO_CMD) == 0) {
		uint8_t temp[100];
		uint8_t mac[6];
		wifi_get_macaddr(STATION_IF, mac);
		os_sprintf(temp, "{"MACSTR"}", MAC2STR(mac));
		my_espconn_sent_headered(link, temp, strlen(temp), dte->header, strlen(dte->header));

	} else {

		result = 0;

	}

	return result;
}

uint8_t ICACHE_FLASH_ATTR
special_uart_cmd(uart_data_to_exec_t *dte) {
	uint8_t result = 1;

	if (my_start_with(CONNECT_TO_WIFI, dte->data) == 0) {

		debug_print_str_uart("WIF:cfg received plis wait \r\n");
		join_access_point(dte->data + sizeof(CONNECT_TO_WIFI) - 1);

	} else {
		result = 0;

	}

	return result;
}
