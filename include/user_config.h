
#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#define SSID "n8F86"
#define SSID_PASSWORD "12345678"

#define IP "192.168.1.170"
#define NETWORK "255.255.255.0"
#define GW "192.168.1.1"

#define MAX_CONNNECTION 	10


#define user_recvTaskPrio        		0
#define user_recvTaskQueueLen     		64

#define tcp_execTaskPrio        		1
#define tcp_execTaskQueueLen    		64

#define uart_execTaskPrio     			2
#define uart_execTaskQueueLen    		64

#define uart_execTaskPrio     			2
#define uart_execTaskQueueLen    		64


enum my_uart_execTaskSignal {
	my_unheadered_msg, my_headered_msg
};

//
enum my_tcp_execTaskSignal {
	my_tcp_msg_comme, my_tcp_disconnect
};

//#define tcp_dcTaskPrio        			9
//#define tcp_dcTaskQueueLen    			64
//


#endif
