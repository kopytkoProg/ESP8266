#include "c_types.h"
#include "driver/uart.h"

uint8_t ICACHE_FLASH_ATTR
find_first(uint8_t s[], uint8_t length, uint8_t c) {

	int16_t r = -1;
	int16_t i = 0;

	for (i = 0; r == -1 && i < length; i++) {
		if (s[i] == c)
			r = i;
	}
	return (uint8_t) (r & 0xff);
}
