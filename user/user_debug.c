/*
 * user_debug.c
 *
 *  Created on: 18 sie 2015
 *      Author: michal
 */

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

at_linkConType *debug = NULL;

void ICACHE_FLASH_ATTR
set_debug_msg_target(at_linkConType *d) {

	debug = d;

}

void ICACHE_FLASH_ATTR
debug_print_str_uart(uint8_t *d) {
	uart0_sendStr(d);
}

void ICACHE_FLASH_ATTR
debug_print_str_tcp(uint8_t *d) {
	debug_print_bfr_tcp(d, strlen(d));
}

void ICACHE_FLASH_ATTR
debug_print_bfr_tcp(uint8_t *d, uint16_t l) {

	if (debug == NULL || debug->free) {
		debug = NULL;
		return;
	}

	if (l < 100)
		my_espconn_sent(debug, d, l);

}

