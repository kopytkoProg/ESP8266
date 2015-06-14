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

//----------------------------------------------------------------------------------
// This part is response for special command {debuging, keep alive} thru TCP
//----------------------------------------------------------------------------------



at_linkConType *debug = NULL;

uint8_t ICACHE_FLASH_ATTR
my_start_with(uint8_t *s1, uint8_t *s2) {

	for (; *s1 == *s2; s1++, s2++) {
		if (*s1 == '\0') {
			return 0;
		}
	}

	if (*s1 == '\0' || *s2 == '\0') {
		return 0;
	}

	return 1;
}

uint8_t ICACHE_FLASH_ATTR
special_cmd(uint8_t *d, uint16_t l, at_linkConType *link) {
	uint8_t result = 0;

	if (l == sizeof(DEBUG_CMD) - 1 && my_start_with(DEBUG_CMD, d) == 0) {
		debug = link;

		result = 1;
	} else if (l == sizeof(KEEP_ALIVE_CMD) - 1 && my_start_with(KEEP_ALIVE_CMD, d) == 0) {
		my_espconn_sent(link, KEEP_ALIVE_CMD, strlen(KEEP_ALIVE_CMD));
		result = 1;
	}

	return result;
}

void ICACHE_FLASH_ATTR
debug_print_str(uint8_t *d) {

	if (debug == NULL || debug->free) {
		debug = NULL;
		return;
	}

	if (strlen(d) < 100)
		my_espconn_sent(debug, d, strlen(d));

}

void ICACHE_FLASH_ATTR
debug_print_bfr(uint8_t *d, uint16_t l) {

	if (debug == NULL || debug->free) {
		debug = NULL;
		return;
	}

	if (l < 100)
		my_espconn_sent(debug, d, l);

}

