#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "user_tcp.h"
#include "user_config.h"
#include "osapi.h"

//----------------------------------------------------------------------------------
static struct espconn *pTcpServer;
// uint8_t next_conn_id = 0;
at_linkConType slot[MAX_CONNNECTION];

//----------------------------------------------------------------------------------
void ICACHE_FLASH_ATTR
init_slots() {

	uint8_t i = 0;
	for (i = 0; i < MAX_CONNNECTION; i++) {
		slot[i].free = TRUE;
		slot[i].linkId = i;
	}
}

int8_t ICACHE_FLASH_ATTR
get_first_free_slot() {

	int8_t i = 0;
	int8_t r = -1;

	for (i = 0; r == -1 && i < MAX_CONNNECTION; i++) {
		if (slot[i].free)
			r = i;
	}
	return r;
}

//----------------------------------------------------------------------------------

/**
 * @brief  Client received callback function.
 * @param  arg: contain the ip link information
 * @param  pdata: received data
 * @param  len: the lenght of received data
 * @retval None
 */
void ICACHE_FLASH_ATTR
at_tcpclient_recv(void *arg, char *pdata, unsigned short len) {
	struct espconn *pespconn = (struct espconn *) arg;
	at_linkConType *s = (at_linkConType *) pespconn->reverse;

	uint8_t buffer[20];

	os_sprintf(buffer, "<%d,%d>:", s->linkId, len);
	uart0_sendStr(buffer);

	uart0_tx_buffer(pdata, len);

}
/**
 * @brief  Tcp server connect repeat callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpserver_recon_cb(void *arg, sint8 errType) {
	struct espconn *pespconn = (struct espconn *) arg;
	at_linkConType *s = (at_linkConType *) pespconn->reverse;
	s->free = TRUE;

	uart0_sendStr("at_tcpserver_recon_cb \n\r");
}

/**
 * @brief  Tcp server disconnect success callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpserver_discon_cb(void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	at_linkConType *s = (at_linkConType *) pespconn->reverse;
	s->free = TRUE;

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

	int8_t first_free_slot = get_first_free_slot();
	if (first_free_slot == -1)
		return;

	slot[first_free_slot].free = FALSE;
	slot[first_free_slot].pCon = pespconn;
	pespconn->reverse = &slot[first_free_slot];

	uart0_sendStr("at_tcpserver_listen \n\r");
	espconn_regist_recvcb(pespconn, at_tcpclient_recv);
	espconn_regist_reconcb(pespconn, at_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, at_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);

}

void ICACHE_FLASH_ATTR
createServer() {

	init_slots();

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
