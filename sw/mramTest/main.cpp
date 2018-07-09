#include <string.h>
#include "stdio.h"

static char* const MRAM_START_ADDR = reinterpret_cast<char* const>(0x60000000);
static const unsigned int MRAM_SIZE = 0x0FFFFFFF;


int main() {
	char buffer[10] = {0};

	memcpy(buffer, MRAM_START_ADDR, 10);

	printf("Before:\n");
	printf("%0.*s\n", 10, buffer);

	memcpy(MRAM_START_ADDR, "Kokosnuss", 10);


	memcpy(buffer, MRAM_START_ADDR, 10);
	printf("After:\n");
	printf("%0.*s\n", 10, buffer);

}
