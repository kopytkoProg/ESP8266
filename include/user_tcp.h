//
typedef struct {
	uint8_t linkId;
	uint8_t free;
	struct espconn *pCon;
} at_linkConType;


#define true            (1)
#define false           (0)
#define TRUE            true
#define FALSE           false

void ICACHE_FLASH_ATTR createServer();
