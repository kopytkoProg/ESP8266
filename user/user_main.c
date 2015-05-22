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

static void loop(os_event_t *events);
void createServer();

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events) {

	// system_os_post(user_procTaskPrio, 0, 0);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init() {

	// ---------------- my uart ----------------
	user_uart_init();
	//Set station mode

	//Set ap settings
	uart0_sendStr("\r\n");
	uart0_sendStr("Hello \n\r");

	uart0_sendStr("\r\n");

	setup_wifi();

	uart0_sendStr("CreatingServer...\r\n");
	createServer();

	system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
	system_os_post(user_procTaskPrio, 0, 0);

}
