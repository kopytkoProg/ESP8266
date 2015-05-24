#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "user_tcp.h"
#include "user_config.h"
#include "osapi.h"
#include "at.h"



/************************************************************************
 * Known issues:
 * - when someone connect as a first client then he get id = 0 and when he send something
 *   and disconnect and new client connect (and no other client) then he get id = 0
 *   so the uart client don have enough time to response to the first client, he respond
 *   to the second client
 *   Solution: use more complex linkId as id.
 ************************************************************************/


//----------------------------------------------------------------------------------
static struct espconn *pTcpServer;
// uint8_t next_conn_id = 0;
at_linkConType slot[MAX_CONNNECTION];

// uint8_t linkId_counter = MAX_CONNNECTION;

//----------------------------------------------------------------------------------
/***
 * pre:		id < MAX_CONNNECTION
 */
at_linkConType * ICACHE_FLASH_ATTR
get_link_by_id(uint8_t id) {
	return &slot[id];
}
/***
 * This function return the link or 0 if link not exist
 */
at_linkConType * ICACHE_FLASH_ATTR
get_link_by_linkId(uint8_t id) {

	uint8_t i = 0;
	for (i = 0; i < MAX_CONNNECTION; i++) {
		if (!slot[i].free && slot[i].linkId == id)
			return &slot[i];
	}

	return 0;
}

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

	/****************************************************************
	 * Data sendet to ESP shuld look like this {[^{}]*} eg: {do something: 1}
	 *
	 ****************************************************************/

	// it is first packet of data from this client
	// the packet shuld start with '{'
	if (s->len == 0 && len > 0 && *pdata != '{') {
		espconn_sent(pespconn, INVALID_START_OF_TRANSMISION, sizeof(INVALID_START_OF_TRANSMISION));
		espconn_disconnect(pespconn);
		return;
	}

	if (len + s->len > MAX_RECEIVE) {
		espconn_sent(pespconn, TO_MANY_DATA, sizeof(TO_MANY_DATA));
		espconn_disconnect(pespconn);
		return;
	}

	if (len > 0) {

		// received data and previous data are copied in to
		// new location
		// old location will be removed

		uint8_t *data = (uint8_t *) os_zalloc(sizeof(uint8_t) * (len + s->len));

		uint16_t i = 0;

		if (s->len > 0) {

			for (i = 0; i < s->len; i++)
				*(data + i) = *(s->data + i);

			os_free(s->data);
		}

		for (i = s->len; i < s->len + len; i++)
			*(data + i) = *(pdata + i - s->len);

		s->data = data;
		s->len = s->len + len;

	}

	//--------
	//	uint8_t buffer[20];
	//	os_sprintf(buffer, "<%d, %d, %d>:", s->linkId, len, s->len);
	//	uart0_sendStr(buffer);
	//--------
	//	uart0_tx_buffer(s->data, s->len);

	// if end of packet reach then send the data to exec

	if (*(s->data + s->len - 1) == '}') {
		uart0_sendStr("END OF PACKET!");

		tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) os_zalloc(sizeof(tcp_data_to_exec_t));
		dte->len = s->len;
		dte->data = s->data;
		dte->link = s;

		// now the executing process have to remove this data
		s->len = 0;

		system_os_post(user_procTaskPrio, 0, (uint32_t) dte);

	}
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
	if (s->len > 0)
		os_free(s->data);
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
	if (s->len > 0)
		os_free(s->data);
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
	slot[first_free_slot].len = 0;
	// slot[first_free_slot].linkId = linkId_counter++;

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
