
#define ECHO	1

void ICACHE_FLASH_ATTR user_uart_init();

typedef struct {
	uint8_t id;
	uint16_t len;
	uint8_t *data;
} uart_data_to_exec_t;
