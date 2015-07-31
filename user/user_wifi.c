#include "user_config.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "utils/str.h"

os_timer_t at_japDelayChack;

//int8_t ICACHE_FLASH_ATTR
//at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen) {
////  assert(pDest!=NULL && pSrc!=NULL);
//
//	char *pTempD = pDest;
//	const char *pTempS = pSrc;
//	int8_t len;
//
//	if (*pTempS != '\"') {
//		return -1;
//	}
//	pTempS++;
//	for (len = 0; len < maxLen; len++) {
//		if (*pTempS == '\"') {
//			*pTempD = '\0';
//			break;
//		} else {
//			*pTempD++ = *pTempS++;
//		}
//	}
//	if (len == maxLen) {
//		return -1;
//	}
//	return len;
//}

void ICACHE_FLASH_ATTR
at_japChack(void *arg) {
	static uint8_t chackTime = 0;
	uint8_t japState;
	char temp[32];

	os_timer_disarm(&at_japDelayChack);
	chackTime++;
	japState = wifi_station_get_connect_status();
	if (japState == STATION_GOT_IP) {
		chackTime = 0;
		uart0_sendStr("wifi: SUCCES \r\n");
		return;
	} else if (chackTime >= 7) {
		wifi_station_disconnect();
		chackTime = 0;
		uart0_sendStr("wifi: FAIL \r\n");
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
at_setupCmdCwjap(char *pPara) {
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
		os_timer_setfn(&at_japDelayChack, (os_timer_func_t *) at_japChack, NULL);
		os_timer_arm(&at_japDelayChack, 3000, 0);

	} else {
		uart0_sendStr("wifi: INVALID CFG \r\n");
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

	uart0_sendStr("Connecting to wifi: ");
	uart0_sendStr(ssid);
	uart0_sendStr("\r\n");

	// Set static IP
	wifi_station_dhcpc_stop();
	struct ip_info pTempIp;
	wifi_get_ip_info(0x00, &pTempIp);

	pTempIp.ip.addr = ipaddr_addr(IP);
	pTempIp.netmask.addr = ipaddr_addr(NETWORK);
	pTempIp.gw.addr = ipaddr_addr(GW);

	if (!wifi_set_ip_info(0x00, &pTempIp)) {
		uart0_sendStr("\r\nWIFI-ERROR-2\r\n");
		wifi_station_dhcpc_start();
	}

	wifi_get_ip_info(0x00, &pTempIp);

	uint8_t t[100];
	os_sprintf(t, "%d.%d.%d.%d\r\n", IP2STR(&pTempIp.ip));
	uart0_sendStr(t);

	uint8 bssid[6];
	wifi_get_macaddr(STATION_IF, bssid);
	os_sprintf(t, "\""MACSTR"\"\r\n", MAC2STR(bssid));

	uart0_sendStr(t);



	// os_timer_arm(&at_japDelayChack, 3000, 0);
}

