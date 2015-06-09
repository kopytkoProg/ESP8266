#include "mem.h"
#include "os_type.h"
#include "utils/fifo_buffer.h"

void ICACHE_FLASH_ATTR
fifo_init(fifo_buffer_t *fifo, uint8_t size /* ,  void (*f)(void*) */) {
	fifo->size = size;
	// fifo->f = f;
	fifo->q = (void *) os_zalloc(sizeof(void *) * size);
	fifo->i = 0;
	fifo->c = 0;

}

static uint8_t ICACHE_FLASH_ATTR
first(fifo_buffer_t *fifo) {
	return ((fifo->i + fifo->size) - fifo->c) % fifo->size;
}

uint8_t ICACHE_FLASH_ATTR
fifo_is_empty(fifo_buffer_t *fifo) {
	return fifo->c == 0;
}

uint8_t ICACHE_FLASH_ATTR
fifo_is_full(fifo_buffer_t *fifo) {
	return fifo->c == fifo->size;
}

/***
 * Return pointer or NULL if empty
 */
void* ICACHE_FLASH_ATTR
fifo_pop(fifo_buffer_t *fifo) {
	void *p = NULL;

	if (!fifo_is_empty(fifo)) {
		p = fifo->q[first(fifo)];
		fifo->c--;
	}

	return p;
}

/***
 * Return pointer or NULL if empty
 */
void* ICACHE_FLASH_ATTR
fifo_first(fifo_buffer_t *fifo) {
	void *p = NULL;

	if (!fifo_is_empty(fifo)) {
		p = fifo->q[first(fifo)];
	}

	return p;
}

int8_t ICACHE_FLASH_ATTR
fifo_push(fifo_buffer_t *fifo, void *p) {

	if (!fifo_is_full(fifo)) {
		fifo->q[fifo->i] = p;
		fifo->i = (fifo->i + 1) % fifo->size;
		fifo->c++;
		return 1;
	}
	return -1;
}
