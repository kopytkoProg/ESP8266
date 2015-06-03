#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "user_wifi.h"
#include "user_tcp.h"
#include "user_uart.h"

os_event_t tcp_execTaskQueue[tcp_execTaskQueueLen];
os_event_t uart_execTaskQueue[uart_execTaskQueueLen];
// os_event_t tcp_dcTaskQueue[tcp_dcTaskQueueLen];

static void tcp_exec(os_event_t *events);
void createServer();

typedef enum {
	Idle, WaitingForResponse
} user_exe_state_t;

user_exe_state_t main_state = Idle;
tcp_data_to_exec_t *dte_in_progres;

#define QUEUE_SIZE  20
#define ERJECTED 	("Rejected!")

uint8_t dte_queue_i = 0;
uint8_t dte_queue_count = 0;
tcp_data_to_exec_t *dte_queue[QUEUE_SIZE];

static uint8_t ICACHE_FLASH_ATTR
first_in_queue() {
	return ((dte_queue_i + QUEUE_SIZE) - dte_queue_count) % QUEUE_SIZE;
}

static int8_t ICACHE_FLASH_ATTR
add_task_to_queue(tcp_data_to_exec_t *dte) {

	if (dte_queue_count >= QUEUE_SIZE)
		return -1;

	dte_queue[dte_queue_i] = dte;
	dte_queue_i = (dte_queue_i + 1) % QUEUE_SIZE;
	dte_queue_count++;

	return 0;
}

static void ICACHE_FLASH_ATTR
remove_tcp_data_to_exec(tcp_data_to_exec_t *dte) {
	if (dte->data != NULL && dte->len > 0)
		os_free(dte->data);
	os_free(dte);
}

static void ICACHE_FLASH_ATTR
exec_data() {

	if (main_state == Idle && dte_queue_count > 0) {
		tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) dte_queue[first_in_queue()];

		// if dte is closing info then let it do
		if (dte->link->free && dte->len > 0) { 				// closed
		// remove because no one get response
			remove_tcp_data_to_exec(dte);
			dte_queue_count--;
			return exec_data();
		}
		main_state = WaitingForResponse;

		at_linkConType *l = (at_linkConType *) dte->link;
		struct espconn *e = (struct espconn *) l->pCon;

		//--------
		uint8_t buffer[30];
		os_sprintf(buffer, "<%d, %d>", l->linkId, dte->len);
		uart0_sendStr(buffer);
		//--------

		if (dte->len > 0)
			uart0_tx_buffer(dte->data, dte->len);

	}
}

static void ICACHE_FLASH_ATTR
response(uint8_t *b, uint16_t size) {

	if (dte_queue_count > 0) {

		tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) dte_queue[first_in_queue()];
		at_linkConType *l = (at_linkConType *) dte->link;
		struct espconn *e = (struct espconn *) l->pCon;

		if (l->free && dte->len > 0) {
			// uart0_sendStr("\r\nERROR: connection closed! \r\n");
			// Do nothing when uart send response and the tcp client have disconnected
		} else if (l->free) {	// when is response for closing info
			// DO NOTHING WITH CONFIRMATION ABOUT TCP CLOSED CONNECTION
		} else {
			my_espconn_sent(l, b, size);
		}

		remove_tcp_data_to_exec(dte);
		dte_queue_count--;
	} else {
		uart0_sendStr("\r\nERROR: no task in queue! \r\n");
		debug_print_str("\r\nERROR: no task in queue! \r\n");
	}
	main_state = Idle;
}
/****************************************************************

 ****************************************************************/
//static void ICACHE_FLASH_ATTR
//tcp_dc(os_event_t *events) {
//	at_linkConType *l = (at_linkConType *) events->par;
//
//
//	uart0_sendStr("\r\nCLOSED ! \r\n");
//	if (add_task_to_queue(dte) == -1) {
//		uart0_sendStr("\r\nERROR: queue is full ! \r\n");
//	}
//
//	exec_data();
//
//}
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

	debug_print_str("\r\n TCP msg: \r\n");
	debug_print_bfr(dte->data, dte->len);

	switch (events->sig) {
	case my_tcp_msg_comme: {

		if (add_task_to_queue(dte) == -1)
			my_espconn_sent(l, ERJECTED, sizeof(ERJECTED));

		exec_data();
	}
		break;
	case my_tcp_disconnect:

		if (add_task_to_queue(dte) == -1) {
			uart0_sendStr("\r\nERROR: queue is full ! \r\n");
			debug_print_str("\r\nERROR: queue is full ! \r\n");
		}
		exec_data();

		break;
	default:
		break;
	}

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

	switch (events->sig) {
	case my_headered_msg:


		debug_print_str("\r\n UART headered msg: \r\n");
		debug_print_bfr(dte->data, dte->len);

		my_espconn_sent(get_link_by_linkId(dte->id), dte->data, dte->len);

		os_free(dte->data);
		os_free(dte);

		break;
	case my_unheadered_msg:

		// uart0_tx_buffer(dte->data, dte->len);

		debug_print_str("\r\n UART unheadered msg: \r\n");
		debug_print_bfr(dte->data, dte->len);

		// Id from uart data is not used (0)
		response(dte->data, dte->len);

		os_free(dte->data);
		os_free(dte);

		exec_data();

		break;
	default:
		break;
	}
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

	system_os_task(tcp_exec, tcp_execTaskPrio, tcp_execTaskQueue, tcp_execTaskQueueLen);
	system_os_task(uart_exec, uart_execTaskPrio, uart_execTaskQueue, uart_execTaskQueueLen);
	//system_os_task(tcp_dc, tcp_dcTaskPrio, tcp_dcTaskQueue, tcp_dcTaskQueueLen);

}
