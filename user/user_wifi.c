#include "user_config.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"

void setup_wifi() {
	char ssid[32] = SSID;
	char password[64] = SSID_PASSWORD;
	struct station_config stationConf;

	wifi_set_opmode(0x1);
	os_memcpy(&stationConf.ssid, ssid, 32);
	os_memcpy(&stationConf.password, password, 64);

	uart0_sendStr("Connecting to wifi: ");
	uart0_sendStr(ssid);
	uart0_sendStr("\r\n");

	if (wifi_station_set_config(&stationConf)) {
		uart0_sendStr("Succes \r\n");
	} else {
		uart0_sendStr("\r\nWIFI-ERROR-1\r\n");
	}


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
}