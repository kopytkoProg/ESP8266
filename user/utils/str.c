#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"


int8_t ICACHE_FLASH_ATTR
at_dataStrCpy(void *pDest, const void *pSrc, int8_t maxLen) {
//  assert(pDest!=NULL && pSrc!=NULL);

	char *pTempD = pDest;
	const char *pTempS = pSrc;
	int8_t len;

	if (*pTempS != '\"') {
		return -1;
	}
	pTempS++;
	for (len = 0; len < maxLen; len++) {
		if (*pTempS == '\"') {
			*pTempD = '\0';
			break;
		} else {
			*pTempD++ = *pTempS++;
		}
	}
	if (len == maxLen) {
		return -1;
	}
	return len;
}

uint8_t ICACHE_FLASH_ATTR
my_start_with(uint8_t *s1, uint8_t *s2) {

	for (; *s1 == *s2; s1++, s2++) {
		if (*s1 == '\0') {
			return 0;
		}
	}

	if (*s1 == '\0' || *s2 == '\0') {
		return 0;
	}

	return 1;
}
