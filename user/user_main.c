#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "at.h"
#include "espconn.h"
#include "mem.h"
#include "user_wifi.h"

os_event_t user_recvTaskQueue[user_recvTaskQueueLen];
os_event_t user_procTaskQueue[user_procTaskQueueLen];

static void loop(os_event_t *events);
void createServer();

static void ICACHE_FLASH_ATTR ///////
user_recvTask(os_event_t *events) {
	uint8_t temp;

	while (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) {

		WRITE_PERI_REG(0X60000914, 0x73); //WTD
		temp = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
		if ((temp != '\n') && (1)) {
			uart_tx_one_char(temp); //display back
		}
	}
	if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_FULL_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR);
	} else if (UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(UART0)) & UART_RXFIFO_TOUT_INT_ST)) {
		WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_TOUT_INT_CLR);
	}

	ETS_UART_INTR_ENABLE();
}

//Main code function
static void ICACHE_FLASH_ATTR
loop(os_event_t *events) {

	// system_os_post(user_procTaskPrio, 0, 0);
}
//----------------------------------------------------------------------------------
static struct espconn *pTcpServer;
/**
 * @brief  Client received callback function.
 * @param  arg: contain the ip link information
 * @param  pdata: received data
 * @param  len: the lenght of received data
 * @retval None
 */
void ICACHE_FLASH_ATTR
at_tcpclient_recv(void *arg, char *pdata, unsigned short len) {
	uart0_sendStr("at_tcpclient_recv: ");
	struct espconn *pespconn = (struct espconn *) arg;
	uart0_tx_buffer(pdata, len);
	espconn_sent(pespconn, pdata, len);

}
/**
 * @brief  Tcp server connect repeat callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpserver_recon_cb(void *arg, sint8 errType) {
	uart0_sendStr("at_tcpserver_recon_cb \n\r");

}

/**
 * @brief  Tcp server disconnect success callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpserver_discon_cb(void *arg) {
	uart0_sendStr("at_tcpserver_discon_cb \n\r");
}

/**
 * @brief  Client send over callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpclient_sent_cb(void *arg) {
	uart0_sendStr("at_tcpclient_sent_cb \n\r");

}

LOCAL void ICACHE_FLASH_ATTR
at_tcpserver_listen(void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	uart0_sendStr("at_tcpserver_listen \n\r");
	espconn_regist_recvcb(pespconn, at_tcpclient_recv);
	espconn_regist_reconcb(pespconn, at_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, at_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);

}

void createServer() {
	pTcpServer = (struct espconn *) os_zalloc(sizeof(struct espconn));
	if (pTcpServer == NULL) {
		uart0_sendStr("TcpServer Failure\r\n");
		return;
	}
	uart0_sendStr("createServer \n\r");

	pTcpServer->type = ESPCONN_TCP;
	pTcpServer->state = ESPCONN_NONE;
	pTcpServer->proto.tcp = (esp_tcp *) os_zalloc(sizeof(esp_tcp));
	pTcpServer->proto.tcp->local_port = 300;
	espconn_regist_connectcb(pTcpServer, at_tcpserver_listen);
	espconn_accept(pTcpServer);
	espconn_regist_time(pTcpServer, 180, 0);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init() {


	// ---------------- my uart ----------------
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	//Set station mode


	//Set ap settings
	uart0_sendStr("\r\n");
	uart0_sendStr("Hello \n\r");

	uart0_sendStr("\r\n");

	setup_wifi();

	uart0_sendStr("CreatingServer...\r\n");
	createServer();

	system_os_task(loop, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
	system_os_task(user_recvTask, user_recvTaskPrio, user_recvTaskQueue, user_recvTaskQueueLen);

	system_os_post(user_procTaskPrio, 0, 0);

}
