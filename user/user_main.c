#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "at.h"
#include "espconn.h"
#include "mem.h"

#include "user_wifi.h"
#include "user_tcp.h"
#include "user_uart.h"

os_event_t user_procTaskQueue[user_procTaskQueueLen];
os_event_t user_uart_procTaskQueue[user_uart_procTaskQueueLen];

static void tcp_exec(os_event_t *events);
void createServer();

/****************************************************************
 * Remember to remove:
 * 		dte->data
 * 		dte
 * Other fields are in use
 ****************************************************************/
static void ICACHE_FLASH_ATTR
tcp_exec(os_event_t *events) {

	tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) events->par;
	at_linkConType *l = (at_linkConType *) dte->link;
	struct espconn *e = (struct espconn *) l->pCon;

	uart0_tx_buffer(dte->data, dte->len);

	if (!l->free)
		espconn_sent(e, "OK", 2);

	os_free(dte->data);
	os_free(dte);
}

/****************************************************************
 * Remember to remove:
 * 		dte->data
 * 		dte
 * Other fields are in use
 ****************************************************************/
static void ICACHE_FLASH_ATTR
uart_exec(os_event_t *events) {
	uart_data_to_exec_t *dte = (uart_data_to_exec_t *) events->par;
	at_linkConType *l = get_link_by_id(dte->id);

	uart0_tx_buffer(dte->data, dte->len);

	if (dte->id >= MAX_CONNNECTION) {
		uart0_sendStr("\r\nERROR: wrong id! \r\n");
		return;
	}

	if (l->free) {
		uart0_sendStr("\r\nERROR: connection closed! \r\n");
		return;
	}

	espconn_sent(l->pCon, dte->data, dte->len);

	os_free(dte->data);
	os_free(dte);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init() {

	// ---------------- my uart ----------------
	user_uart_init();
	//Set station mode

	//Set ap settings
	uart0_sendStr("\r\n");
	uart0_sendStr("Hello \r\n");

	uart0_sendStr("\r\n");

	setup_wifi();

	uart0_sendStr("CreatingServer...\r\n");
	createServer();

	system_os_task(tcp_exec, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
	system_os_task(uart_exec, user_uart_procTaskPrio, user_uart_procTaskQueue, user_uart_procTaskQueueLen);

}
