#include "user_config.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "utils/str.h"
#include "user_debug.h"
os_timer_t at_japDelayChack;


void ICACHE_FLASH_ATTR
check_wifi_station_status(void *arg) {
	static uint8_t chackTime = 0;
	uint8_t japState;
	char temp[32];

	os_timer_disarm(&at_japDelayChack);
	chackTime++;
	japState = wifi_station_get_connect_status();
	if (japState == STATION_GOT_IP) {
		chackTime = 0;
		debug_print_str_uart("wifi: SUCCES \r\n");
		return;
	} else if (chackTime >= 7) {
		wifi_station_disconnect();
		chackTime = 0;
		debug_print_str_uart("wifi: FAIL \r\n");
		return;
	}
	os_timer_arm(&at_japDelayChack, 2000, 0);

}

/**
 * @brief  Setup commad of join to wifi ap.
 * @param  pPara: AT input param
 * @retval None
 */
void ICACHE_FLASH_ATTR
join_access_point(char *pPara) {
	//at_setupCmdCwjap(0, "\"n8F86\",\"12345678\"");
	char temp[64];
	struct station_config stationConf;

	int8_t len;

	os_bzero(&stationConf, sizeof(struct station_config));

	pPara++;
	len = at_dataStrCpy(&stationConf.ssid, pPara, 32);
	if (len != -1) {
		pPara += (len + 3);
		len = at_dataStrCpy(&stationConf.password, pPara, 64);
	}
	if (len != -1) {

		wifi_station_disconnect();
		ETS_UART_INTR_DISABLE();
		wifi_station_set_config(&stationConf);
		ETS_UART_INTR_ENABLE();
		wifi_station_connect();

		os_timer_disarm(&at_japDelayChack);
		os_timer_setfn(&at_japDelayChack, (os_timer_func_t *) check_wifi_station_status, NULL);
		os_timer_arm(&at_japDelayChack, 3000, 0);

	} else {
		debug_print_str_uart("wifi: INVALID CFG \r\n");
	}
}

void ICACHE_FLASH_ATTR
setup_wifi() {
	char ssid[32];		// = SSID;
	char password[64];		// = SSID_PASSWORD;
	struct station_config stationConf;

	/*
	 * uncomment and set proper data if you want to set wifi cfg
	 *
	*/
	// at_setupCmdCwjap("=\"n8F86\",\"12345678\"");

	os_bzero(ssid, sizeof(ssid));
	os_bzero(password, sizeof(password));

	wifi_station_set_auto_connect(1);
	wifi_set_opmode(0x1);
	wifi_station_get_config(&stationConf);

	os_memcpy(ssid, &stationConf.ssid, 32);
	os_memcpy(password, &stationConf.password, 64);

	debug_print_str_uart("Connecting to wifi: ");
	debug_print_str_uart(ssid);
	debug_print_str_uart("\r\n");

	// Set static IP
	wifi_station_dhcpc_stop();
	struct ip_info pTempIp;
	wifi_get_ip_info(0x00, &pTempIp);

	pTempIp.ip.addr = ipaddr_addr(IP);
	pTempIp.netmask.addr = ipaddr_addr(NETWORK);
	pTempIp.gw.addr = ipaddr_addr(GW);

	if (!wifi_set_ip_info(0x00, &pTempIp)) {
		debug_print_str_uart("\r\nWIFI-ERROR-2\r\n");
		wifi_station_dhcpc_start();
	}

	wifi_get_ip_info(0x00, &pTempIp);

	uint8_t t[100];
	os_sprintf(t, "%d.%d.%d.%d\r\n", IP2STR(&pTempIp.ip));
	debug_print_str_uart(t);

	uint8 bssid[6];
	wifi_get_macaddr(STATION_IF, bssid);
	os_sprintf(t, "\""MACSTR"\"\r\n", MAC2STR(bssid));

	debug_print_str_uart(t);



	// os_timer_arm(&at_japDelayChack, 3000, 0);
}

