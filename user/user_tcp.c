#include "user_interface.h"
#include "c_types.h"
#include "driver/uart.h"
#include "espconn.h"
#include "mem.h"
#include "user_tcp.h"
#include "user_config.h"
#include "osapi.h"
#include "utils/fifo_buffer.h"
#include "user_debug.h"


//----------------------------------------------------------------------------------
static struct espconn *pTcpServer;
// uint8_t next_conn_id = 0;
at_linkConType slot[CONNECTION_SLOTS_SIZE];

// uint8_t linkId_counter = MAX_CONNNECTION;

static void ICACHE_FLASH_ATTR check_if_first_faill();

//----------------------------------------------------------------------------------
// This part response for connections (slots)
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
	for (i = 0; i < CONNECTION_SLOTS_SIZE; i++) {
		if (slot[i].linkId == id)
			return &slot[i];
	}

	return 0;
}

void ICACHE_FLASH_ATTR
init_slots() {

	uint8_t i = 0;
	for (i = 0; i < CONNECTION_SLOTS_SIZE; i++) {
		slot[i].free = TRUE;
		slot[i].linkId = i;

	}
}

uint8_t next_slot = 0;
int8_t ICACHE_FLASH_ATTR
get_first_free_slot() {

	int8_t i = 0;
	int8_t r = -1;

	for (i = 0; r == -1 && i < CONNECTION_SLOTS_SIZE; i++) {
		if (slot[(i + next_slot) % CONNECTION_SLOTS_SIZE].free)
			r = (i + next_slot) % CONNECTION_SLOTS_SIZE;
	}

	next_slot = r + 1;
	return r;
}

//----------------------------------------------------------------------------------
// This part is response for buffering and sending message.
// When user call two time my_espconn_sent then the first packet is sent and second packet wait until first one successful sent or fail
// Msg fail when first packet in queue waiting for successful sent confirmation and the connection is closed
//----------------------------------------------------------------------------------

typedef enum {
	Idle, WaitingForSend
} my_send_state_t;

typedef struct {
	uint16_t length;
	uint8_t *data;
	at_linkConType *l;
} my_send_queue_item_t;

fifo_buffer_t msg_fifo_buffer;
fifo_buffer_t *msg_queue = &msg_fifo_buffer;

my_send_state_t my_send_state = Idle;

static void ICACHE_FLASH_ATTR
add_to_sent_queue(at_linkConType *l, uint8_t *data, uint16_t length) {

	if (!fifo_is_full(msg_queue)) {

		my_send_queue_item_t *i = (my_send_queue_item_t *) os_zalloc(sizeof(my_send_queue_item_t));

		uint16_t j = 0;
		i->data = (uint8_t *) os_zalloc(length);
		i->l = l;
		i->length = length;

		for (j = 0; j < length; ++j)
			*(i->data + j) = *(data + j);

		fifo_push(msg_queue, i);

	}

}

/**
 * Remove first task in queue and decrement size
 */
static void ICACHE_FLASH_ATTR
remove_first() {

	if (my_send_state == WaitingForSend && !fifo_is_empty(msg_queue)) {
		my_send_queue_item_t *i = (my_send_queue_item_t *) fifo_pop(msg_queue);

		os_free(i->data);
		os_free(i);

		my_send_state = Idle;
	}

}

/***
 * Send data if in Idle state
 */
static void ICACHE_FLASH_ATTR
my_sent_next() {

	if (my_send_state == Idle && !fifo_is_empty(msg_queue)) {

		my_send_state = WaitingForSend;
		my_send_queue_item_t *i = (my_send_queue_item_t *) fifo_first(msg_queue);

		// when the connection is closed do not send the data
		if (i->l->free) {
			remove_first();
			my_sent_next();
		} else {
			espconn_sent(i->l->pCon, i->data, i->length);
		}
	}

}

void ICACHE_FLASH_ATTR
my_espconn_sent(at_linkConType *l, uint8_t *data, uint16_t length) {

	// os_intr_lock();

	add_to_sent_queue(l, data, length);
	my_sent_next();

	// os_intr_unlock();
}

static void ICACHE_FLASH_ATTR
on_task_serviced() {
	// os_intr_lock();

	remove_first();
	my_sent_next();

	// os_intr_unlock();

}
/***
 * Remove first task from queue when the task is waiting for successful send confirmation and the connection linked with
 * the task is closed (because fail ?).
 */
static void ICACHE_FLASH_ATTR
check_if_first_faill() {
	if (my_send_state == WaitingForSend) {
		my_send_queue_item_t *i = (my_send_queue_item_t *) fifo_first(msg_queue);
		if (i->l->free) {
			on_task_serviced();
		}
	}
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
		my_espconn_sent(s, INVALID_START_OF_TRANSMISION, sizeof(INVALID_START_OF_TRANSMISION));
		espconn_disconnect(pespconn);
		return;
	}

	if (len + s->len > MAX_RECEIVE) {
		my_espconn_sent(s, TO_MANY_DATA, sizeof(TO_MANY_DATA));
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

	// if end of packet reach then send the data to exec

	if (*(s->data + s->len - 1) == '}') {

		// uart0_sendStr("END OF PACKET!");
		// debug_print_str("TCP: Packet received: \r\n");
		// debug_print_bfr(s->data, s->len);

		tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) os_zalloc(sizeof(tcp_data_to_exec_t));
		dte->len = s->len;
		dte->data = s->data;
		dte->link = s;

		// now the executing process have to remove this data
		s->len = 0;

		system_os_post(my_taskPrio, my_tcp_msg_comme, (uint32_t) dte);

	}
}

static void ICACHE_FLASH_ATTR
disconnect(void *arg) {
	struct espconn *pespconn = (struct espconn *) arg;
	at_linkConType *s = (at_linkConType *) pespconn->reverse;

	s->free = TRUE;
	if (s->len > 0)
		os_free(s->data);

	tcp_data_to_exec_t *dte = (tcp_data_to_exec_t *) os_zalloc(sizeof(tcp_data_to_exec_t));
	dte->len = 0;
	dte->data = NULL;
	dte->link = s;

	check_if_first_faill();

	system_os_post(my_taskPrio, my_tcp_disconnect, (uint32_t) dte);
}

/**
 * @brief  Tcp server connect repeat callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpserver_recon_cb(void *arg, sint8 errType) {
	debug_print_str("at_tcpserver_recon_cb");

	disconnect(arg);
}

/**
 * @brief  Tcp server disconnect success callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpserver_discon_cb(void *arg) {
	debug_print_str("at_tcpserver_discon_cb");
	disconnect(arg);

}

/**
 * @brief  Client send over callback function.
 * @param  arg: contain the ip link information
 * @retval None
 */
static void ICACHE_FLASH_ATTR
at_tcpclient_sent_cb(void *arg) {
	// debug_print_str("at_tcpclient_sent_cb \n\r");
	on_task_serviced();
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

	debug_print_str("at_tcpserver_listen \n\r");
	espconn_regist_recvcb(pespconn, at_tcpclient_recv);
	espconn_regist_reconcb(pespconn, at_tcpserver_recon_cb);
	espconn_regist_disconcb(pespconn, at_tcpserver_discon_cb);
	espconn_regist_sentcb(pespconn, at_tcpclient_sent_cb);

}

void ICACHE_FLASH_ATTR
createServer() {

	init_slots();
	fifo_init(msg_queue, SENT_QUEUE_LENGTH);

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
