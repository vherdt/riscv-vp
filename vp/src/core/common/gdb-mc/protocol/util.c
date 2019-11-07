#include <err.h>
#include <stddef.h>
#include <stdlib.h>

#include "parser.h"
#include "fns.h"

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
