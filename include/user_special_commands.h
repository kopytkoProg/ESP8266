#define DEBUG_CMD 				("{debug-esp8266}")
#define KEEP_ALIVE_CMD 			("{keepAlive-esp8266}")
#define SCAN_NETWORK 			("{scanNetwork-esp8266}")
#define MAC_INFO 				("{getMacInfo-esp8266}")



uint8_t ICACHE_FLASH_ATTR special_cmd(tcp_data_to_exec_t *dte);
