//
typedef struct {
	uint8_t linkId;
	uint8_t free;
	struct espconn *pCon;
	uint8_t *data;
	uint16_t len;
} at_linkConType;

typedef struct {
	uint8_t *data;
	uint16_t len;
	at_linkConType *link;
} tcp_data_to_exec_t;

#define true            (1)
#define false           (0)
#define TRUE            true
#define FALSE           false

#define MAX_RECEIVE  	100
#define TO_MANY_DATA 					("ERR: TO MANY DATA!")
#define INVALID_START_OF_TRANSMISION 	("ERR: INVALID START OF TRANSMISION!")

void ICACHE_FLASH_ATTR createServer();
at_linkConType * ICACHE_FLASH_ATTR get_link_by_id(uint8_t id);
at_linkConType * ICACHE_FLASH_ATTR get_link_by_linkId(uint8_t id);
