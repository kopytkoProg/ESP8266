#define DEBUG_CMD 				("{debug-esp8266}")
#define KEEP_ALIVE_CMD 			("{keepAlive-esp8266}")

uint8_t ICACHE_FLASH_ATTR special_cmd(uint8_t *d, uint16_t l, at_linkConType *link);
