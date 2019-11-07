#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "fns.h"

static int
calc_csum(char *data)
{
	size_t i;
	int csum = 0;

	for (i = 0; i < strlen(data); i++)
		csum += (int)data[i];

	return csum % 256;
}

bool
gdb_is_valid(gdb_packet_t *pkt)
{
	int ret;
	int expcsum;
	char strcsum[GDB_CSUM_LEN + 1]; /* +1 for snprintf nullbyte */

	if (!pkt->data)
		return true;
	expcsum = calc_csum(pkt->data);

	ret = snprintf(strcsum, sizeof(strcsum), "%x", expcsum);
	assert(ret == GDB_CSUM_LEN);

	return !strncmp(pkt->csum, strcsum, GDB_CSUM_LEN);
}

void *
xmalloc(size_t size)
{
	void *r;

	if (!(r = malloc(size)))
		err(EXIT_FAILURE, "malloc failed");

	return r;
}

void
gdb_free_packet(gdb_packet_t *pkt)
{
	if (pkt->data)
		free(pkt->data);
	free(pkt);
}
