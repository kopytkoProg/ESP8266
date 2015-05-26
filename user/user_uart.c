#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "driver/uart.h"
#include "user_uart.h"
#include "stdlib.h"
#include "osapi.h"
#include "user_config.h"
#include "user_utils.h"

os_event_t user_recvTaskQueue[user_recvTaskQueueLen];

// will be code to reasembly msg from uart
#define BUFFER_SIZE  	100

uint8_t buffer[BUFFER_SIZE];
uint8_t p = 0;
uint8_t id = 0;
uint8_t size = 0;

enum enum_state {
	waiting_for_cmd, receiving_data
};

enum enum_state state = waiting_for_cmd;

static void ICACHE_FLASH_ATTR
on_char_come(uint8_t c) {

	switch (state) {
	case waiting_for_cmd:

		/****************************************************************
		 * Command to send data to the remote client
		 * receive <id, size> thru uart
		 * id - of connection
		 * size - of data to receive by uart and send to client
		 ****************************************************************/

		if ((p || c == '<') && p < BUFFER_SIZE) {
			buffer[p++] = c;
		}

		if (p && buffer[p - 1] == '>') {

			id = atoi((char *) (buffer + 1));

			size = atoi((char *) (buffer + 1 + find_first(buffer, p, ',')));

			if (ECHO) {
				os_sprintf(buffer, "\r\n<id: %d, size: %d, coma: %d>", id, size, find_first(buffer, p, ','));
				uart0_sendStr(buffer);
			}

			if (id >= 0 && id < MAX_CONNNECTION && size > 0 && size < BUFFER_SIZE)
				state = receiving_data;
			else
				uart0_sendStr("\r\nERROR: invalid id or size! \r\n");

			p = 0;
		}

		break;
	case receiving_data:
		//--------------------------------------------------------
		// uart0_sendStr("TIME FOR DATA! ;)");
		buffer[p++] = c;

		if (p >= size) {
			// done SEND data !

			uart_data_to_exec_t *dte = (uart_data_to_exec_t *) os_zalloc(sizeof(uart_data_to_exec_t));
			uint8_t *d = (uint8_t *) os_zalloc(size);

			dte->data = d;
			dte->len = size;
			dte->id = id;

			uint8_t i = 0;
			for (i = 0; i < size; i++)
				*(d + i) = buffer[i];

//			if (ECHO)
//				uart0_tx_buffer(buffer, size);
			state = waiting_for_cmd;
			p = 0;


			system_os_post(uart_execTaskPrio, 0,(uint32_t) dte);
		}

		break;
	default:
		//--------------------------------------------------------
		break;
	}

}

static void ICACHE_FLASH_ATTR
user_recvTask(os_event_t *events) {
	uint8_t temp;

	while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {

		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		temp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		if ((temp != '\n') && (ECHO)) {
			uart_tx_one_char(temp); //display back
		}
		on_char_come(temp);
	}
	if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	} else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}

	ETS_UART_INTR_ENABLE();
}

void ICACHE_FLASH_ATTR ///////
user_uart_init() {
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	system_os_task(user_recvTask, user_recvTaskPrio, user_recvTaskQueue, user_recvTaskQueueLen);
}
