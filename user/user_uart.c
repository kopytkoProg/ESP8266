#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "driver/uart.h"
#include "at.h"
#include "user_uart.h"
#include "stdlib.h"
#include "osapi.h"

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
		//--------------------------------------------------------
		if ((p || c == '>') && p < BUFFER_SIZE) {
			buffer[p++] = c;
		}

		if (p && buffer[p - 1] == '<') {

			id = atoi((char *) (buffer + 1));
			size = atoi((char *) (buffer + 1 + 3 + 1));

			os_sprintf(buffer, "id: %d, size: %d", id, size);
			uart0_sendStr(buffer);
			p = 0;
			state = receiving_data;
		}

		break;
	case receiving_data:
		//--------------------------------------------------------
		uart0_sendStr("TIME FOR DATA! ;)");
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
