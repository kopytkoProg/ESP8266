#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"


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

void ICACHE_FLASH_ATTR
createServer() {
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
