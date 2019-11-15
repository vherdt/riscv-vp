#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"
#include "protocol.h"
#include "fns.h"

static gdb_command_t *
gdb_new_cmd(char *name, gdb_argument_t type)
{
	gdb_command_t *cmd;

	cmd = malloc(sizeof(*cmd));
	cmd->name = name;
	cmd->type = type;

	return cmd;
}

void
gdb_free_cmd(gdb_command_t *cmd)
{
	free(cmd->name);
	free(cmd);
}

static mpc_val_t *
gdbf_ignarg(mpc_val_t* xs)
{
	int i;
	char *str, *sep;
	gdb_command_t *cmd;

	str = (char *)xs;
	if ((sep = strchr(str, ':')))
		*sep = '\0';

	cmd = gdb_new_cmd(xstrdup(str), GDB_ARG_NONE);
	free(xs);

	return cmd;
}

static mpc_val_t *
gdbf_negative(mpc_val_t *val)
{
	int *neg;

	assert(!strcmp((char*)val, "-1"));
	neg = xmalloc(sizeof(*neg));
	*neg = -1;

	free(val);
	return neg;
}

static mpc_val_t *
gdbf_thread_id(int n, mpc_val_t** xs)
{
	int *arg1, *arg2;
	gdb_thread_t *id;

	assert(n == 2);

	id = xmalloc(sizeof(*id));
	arg1 = (int*)xs[0];
	arg2 = (int*)xs[1];

	if (arg1 != NULL)
		id->pid = *arg1;
	else
		id->pid = GDB_THREAD_UNSET;
	id->tid = *arg2;

	free(xs[0]);
	free(xs[1]);

	return id;
}

static mpc_parser_t *
gdb_hexid(void)
{
	/* Positive numbers with a target-specific interpretation,
	 * formatted as big-endian hex strings. Can also be a literal
	 * '-1' to indicate all threads, or '0' to pick any thread. */
	return mpc_or(2, mpc_hex(),
	              mpc_apply(mpc_string("-1"), gdbf_negative));
}

static mpc_parser_t *
gdb_thread_id(void)
{
	mpc_parser_t *pid, *tid;

	tid = gdb_hexid();
	pid = mpc_and(3, mpcf_snd_free, mpc_char('p'),
	              gdb_hexid(), mpc_char('.'), free, free);

	return mpc_and(2, gdbf_thread_id, mpc_maybe(pid), tid, free);
}

static mpc_val_t *
gdbf_packet_h(int n, mpc_val_t** xs)
{
	gdb_command_t *cmd;

	assert(n == 3);
	assert(*((char*)xs[0]) == 'H');

	cmd = gdb_new_cmd((char *)xs[0], GDB_ARG_H);
	cmd->v.hcmd.op = *((char*)xs[1]);
	cmd->v.hcmd.id = *((gdb_thread_t*)xs[2]);

	free(xs[1]);
	free(xs[2]);

	return cmd;
}

mpc_parser_t *
gdb_packet_h(void)
{
	mpc_parser_t *op, *id;

	op = mpc_any(); /* (‘m’, ‘M’, ‘g’, ‘G’, et.al.). */
	id = gdb_thread_id();

	return mpc_and(3, gdbf_packet_h, mpc_char('H'), op, id, free, free);
}

mpc_parser_t *
gdb_any(void)
{
	return mpc_apply(mpc_many1(mpcf_strfold, mpc_any()), gdbf_ignarg);
}

static mpc_parser_t *
gdb_parse_stage2(void)
{
	return mpc_or(2, gdb_packet_h(), gdb_any());
}

gdb_command_t *
gdb_parse_cmd(gdb_packet_t *pkt)
{
	char *data;
	mpc_result_t r;
	mpc_parser_t *par;
	gdb_command_t *cmd;

	cmd = NULL;
	par = gdb_parse_stage2();

	if (!gdb_is_valid(pkt))
		return NULL;
	if (!(data = gdb_decode_runlen(pkt->data)))
		return NULL;

	if (mpc_parse("<packet>", data, par, &r)) {
		cmd = (gdb_command_t *)r.output;
	} else {
#ifdef GDB_PARSER_DEBUG
		mpc_err_print(r.error);
#endif
		mpc_err_delete(r.error);
	}

	mpc_cleanup(1, par);
	return cmd;
}
