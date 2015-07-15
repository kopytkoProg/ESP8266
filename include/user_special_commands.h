#define DEBUG_CMD 				("{debug-esp8266}")
#define KEEP_ALIVE_CMD 			("{keepAlive-esp8266}")
#define SCAN_NETWORK_CMD 		("{scanNetwork-esp8266}")
#define MAC_INFO_CMD 			("{getMacInfo-esp8266}")

//=====================================================================

#define ASYNC_HEADER_WIFI_INFO 			("wifiInfo-esp8266")




uint8_t ICACHE_FLASH_ATTR special_cmd(tcp_data_to_exec_t *dte);
