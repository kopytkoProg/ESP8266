

#ifndef __USER_FIFO_BUFFER__

#define __USER_FIFO_BUFFER__

typedef struct {
	uint8_t size;
	void* (*q);
	// void (*f)(void*);
	uint8_t i;
	uint8_t c;

} fifo_buffer_t;

void ICACHE_FLASH_ATTR fifo_init(fifo_buffer_t *fifo, uint8_t size /* , void (*f)(void*) */);
int8_t ICACHE_FLASH_ATTR fifo_push(fifo_buffer_t *fifo, void *p);
void* ICACHE_FLASH_ATTR fifo_pop(fifo_buffer_t *fifo);
void* ICACHE_FLASH_ATTR fifo_first(fifo_buffer_t *fifo);

uint8_t ICACHE_FLASH_ATTR fifo_is_empty(fifo_buffer_t *fifo);
uint8_t ICACHE_FLASH_ATTR fifo_is_full(fifo_buffer_t *fifo);

#endif
