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
#include "utils/fifo_buffer.h"
#include "user_debug.h"
#include "user_special_commands.h"

// te function is here becaus of bug, more about this here http://bbs.espressif.com/viewtopic.php?t=486&p=1840
void user_rf_pre_init(void) {
}

// os_event_t tcp_execTaskQueue[tcp_execTaskQueueLen];
// os_event_t uart_execTaskQueue[uart_execTaskQueueLen];
os_event_t my_taskQueue[my_taskkQueueLen];

// os_event_t tcp_dcTaskQueue[tcp_dcTaskQueueLen];
static void tcp_exec(os_event_t *events);
void createServer();

typedef enum {
	Idle, WaitingForResponse
} user_exe_state_t;

user_exe_state_t main_state = Idle;
tcp_data_to_exec_t *dte_in_progres;

#define QUEUE_SIZE  40
#define ERJECTED 	("Rejected!")

fifo_buffer_t fifo_buffer;
fifo_buffer_t *queue = &fifo_buffer;

static void ICACHE_FLASH_ATTR
remove_tcp_data_to_exec(tcp_data_to_exec_t *dte) {
	if (dte->data != NULL && dte->len > 0)
		os_free(dte->data);
	if (dte->header != NULL)
		os_free(dte->header);
	if (dte->content != NULL)
		os_free(dte->content);

	os_free(dte);
}

static void ICACHE_FLASH_ATTR
exec_data() {

	if (main_state == Idle && !fifo_is_empty(queue)) {
		tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) fifo_first(queue);
		if (dte == NULL)
			debug_print_str_uart("\r\nERROR:NULL! \r\n");

		// if dte is closing info then let it do
		if (dte->link->free && dte->len > 0) { 				// closed
		// remove because no one get response
			fifo_pop(queue);
			remove_tcp_data_to_exec(dte);
			return exec_data();
		}
		main_state = WaitingForResponse;

		at_linkConType *l = (at_linkConType *) dte->link;
		struct espconn *e = (struct espconn *) l->pCon;

		uint8_t *b = dte->data;
		uint8_t len = dte->len;

		if (dte->content != NULL) {
			b = dte->content;
			len = strlen(b);
		}

		//--------
		uint8_t buffer[30];
		os_sprintf(buffer, "<%d, %d>", l->linkId, len);
		uart0_sendStr(buffer);
		//--------

		if (len > 0)
			uart0_tx_buffer(b, len);

	}
}
/**
 * Uart response for tcp request
 */
static void ICACHE_FLASH_ATTR
response(uint8_t *b, uint16_t size) {

	if (!fifo_is_empty(queue)) {

		tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) fifo_pop(queue);
		at_linkConType *l = (at_linkConType *) dte->link;
		struct espconn *e = (struct espconn *) l->pCon;

		if (l->free && dte->len > 0) {
			// uart0_sendStr("\r\nERROR: connection closed! \r\n");
			// Do nothing when uart send response and the tcp client have disconnected
		} else if (l->free) {	// when is response for closing info
			// DO NOTHING WITH CONFIRMATION ABOUT TCP CLOSED CONNECTION
		} else {
			my_espconn_sent_headered(l, b, size, dte->header, strlen(dte->header));
		}

		remove_tcp_data_to_exec(dte);

	} else {
		debug_print_str_uart("\r\nERROR: no task in queue! \r\n");
		debug_print_str_tcp("\r\nERROR: no task in queue! \r\n");
	}
	main_state = Idle;
}

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

	debug_print_str_tcp("\r\n TCP msg: \r\n");
	debug_print_bfr_tcp(dte->data, dte->len);

	switch (events->sig) {
	case my_tcp_msg_comme: {

		if (special_cmd(dte)) {
			remove_tcp_data_to_exec(dte);
		} else {

			if (fifo_push(queue, dte) == -1)
				my_espconn_sent(l, ERJECTED, sizeof(ERJECTED));
			else
				exec_data();
		}
	}
		break;
	case my_tcp_disconnect:

		if (fifo_push(queue, dte) == -1) {
			debug_print_str_uart("\r\nERROR: queue is full ! \r\n");
			debug_print_str_tcp("\r\nERROR: queue is full ! \r\n");
			remove_tcp_data_to_exec(dte);
		} else
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
	case my_uart_headered_msg:

		debug_print_str_tcp("\r\n UART headered msg: \r\n");
		debug_print_bfr_tcp(dte->data, dte->len);

		my_espconn_sent(get_link_by_linkId(dte->id), dte->data, dte->len);

		os_free(dte->data);
		os_free(dte);

		break;
	case my_uart_unheadered_msg:

		debug_print_str_tcp("\r\n UART unheadered msg: \r\n");
		debug_print_bfr_tcp(dte->data, dte->len);

		// if it is no uart special command
		if (!special_uart_cmd(dte)) {
			// Id from uart data is not used (0)
			response(dte->data, dte->len);
		}

		os_free(dte->data);
		os_free(dte);

		exec_data();

		break;
	default:
		break;
	}
}

static void ICACHE_FLASH_ATTR
task(os_event_t *events) {

	switch (events->sig) {
	case my_tcp_msg_comme:

		tcp_exec(events);

		break;
	case my_tcp_disconnect:

		tcp_exec(events);

		break;
	case my_uart_unheadered_msg:

		uart_exec(events);

		break;
	case my_uart_headered_msg:

		uart_exec(events);

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

	fifo_init(queue, QUEUE_SIZE);

	//Set ap settings
	debug_print_str_uart("\r\n");
	debug_print_str_uart("Hello \r\n");

	debug_print_str_uart("\r\n");

	setup_wifi();

	debug_print_str_uart("CreatingServer...\r\n");
	createServer();

	system_os_task(task, my_taskPrio, my_taskQueue, my_taskkQueueLen);

}
