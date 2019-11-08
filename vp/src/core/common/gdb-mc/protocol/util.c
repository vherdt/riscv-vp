#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parser.h"
#include "fns.h"

#define GDB_ESCAPE_CHAR '}'
#define GDB_ESCAPE_BYTE 0x20

static int
calc_csum(char *data)
{
	size_t i;
	int csum = 0;

	for (i = 0; i < strlen(data); i++)
		csum += (int)data[i];

	return csum % 256;
}

char *
gdb_unescape(char *data)
{
	size_t i, nlen;
	char *ndat;
	bool esc;

	ndat = xmalloc(strlen(data));
	nlen = 0;

	for (esc = false, i = 0; i < strlen(data); i++) {
		if (data[i] == GDB_ESCAPE_CHAR) {
			esc = true;
			continue;
		}

		ndat[nlen++] = (esc) ? data[i] ^ GDB_ESCAPE_BYTE : data[i];
		esc = false;
	}

	/* shrink to actual size */
	ndat = xrealloc(ndat, nlen + 1);
	ndat[nlen] = '\0';

	return ndat;
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
xrealloc(void *ptr, size_t size)
{
	void *r;

	if (!(r = realloc(ptr, size)))
		err(EXIT_FAILURE, "realloc failed");

	return r;
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
