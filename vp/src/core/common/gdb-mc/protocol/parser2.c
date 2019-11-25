#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "mpc.h"
#include "parser2.h"
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
	gdb_vcont_t *parent, *next;

	if (cmd->type == GDB_ARG_VCONT) {
		parent = cmd->v.vval;
		while (parent) {
			next = parent->next;
			free(parent);
			parent = next;
		}
	}

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

gdbf_fold(h, GDB_ARG_H, GDBF_ARG_HCMD)

mpc_parser_t *
gdb_packet_h(void)
{
	mpc_parser_t *op, *id;

	op = mpc_any(); /* (‘m’, ‘M’, ‘g’, ‘G’, et.al.). */
	id = gdb_thread_id();

	return mpc_and(3, gdbf_packet_h, mpc_char('H'), op, id, free, free);
}

gdbf_fold(p, GDB_ARG_INT, GDBF_ARG_INT)

static mpc_parser_t *
gdb_packet_p(void)
{
	return mpc_and(2, gdbf_packet_p, mpc_char('p'), mpc_hex(), free);
}

static mpc_val_t *
gdbf_vcont_action(int n, mpc_val_t **xs)
{
	char *op, *actstr;
	size_t actlen;
	gdb_vcont_t *vcont;

	assert(n == 2);

	actstr = (char *)xs[0];
	actlen = strlen(actstr);

	vcont = xmalloc(sizeof(*vcont));
	vcont->action = *actstr;
	vcont->next = NULL;

	if (actlen == 1)
		vcont->sig = -1;
	else if (actlen == 3)
		vcont->sig = strtol(actstr + 1, NULL, 16);
	else
		assert(0);

	/* may be a null pointer */
	vcont->thread = (gdb_thread_t *)xs[1];

	free(xs[0]);
	free(xs[1]);

	return vcont;
}

static mpc_val_t *
gdbf_vcont(int n, mpc_val_t **xs)
{
	size_t i;
	gdb_vcont_t *vcont;

	assert(n >= 1);

	for (vcont = (gdb_vcont_t *)xs[0], i = 1; i < n; i++, vcont = vcont->next)
		vcont->next = (gdb_vcont_t *)xs[i];
	vcont->next = NULL;

	return vcont;
}

gdbf_fold(vcont, GDB_ARG_VCONT, GDBF_ARG_VCONT)

static mpc_parser_t *
gdb_packet_vcont(void)
{
	mpc_parser_t *action0, *action1, *action, *args, *thread;

	/* TODO: add support for r command */

	action0 = mpc_oneof("cst");
	action1 = mpc_and(2, mpcf_strfold, mpc_oneof("CS"), mpc_hexdigits());

	thread = mpc_and(2, mpcf_snd_free, mpc_char(':'), gdb_thread_id(), free);
	action = mpc_and(2, gdbf_vcont_action, mpc_or(2, action0, action1),
	                 mpc_maybe(thread), free); /* destructor incorrect but unused */

	args = mpc_many1(gdbf_vcont, mpc_and(2, mpcf_snd_free, mpc_char(';'), action, free));
	return mpc_and(2, gdbf_packet_vcont, mpc_string("vCont"), args, free);
}

gdbf_fold(m, GDB_ARG_MEMORY, GDBF_ARG_MEMORY)

static mpc_parser_t *
gdb_packet_m(void)
{
	return mpc_and(4, gdbf_packet_m, mpc_string("m"),
	               mpc_hex(), mpc_char(','), mpc_hex(), free, free, free);
}

mpc_parser_t *
gdb_any(void)
{
	return mpc_apply(mpc_many1(mpcf_strfold, mpc_any()), gdbf_ignarg);
}

static mpc_parser_t *
gdb_cmd(mpc_parser_t *cmd)
{
	return mpc_and(2, mpcf_fst_free, cmd, mpc_eoi(), gdb_free_cmd);
}

static mpc_parser_t *
gdb_parse_stage2(void)
{
	return mpc_or(5,
	              gdb_cmd(gdb_packet_h()),
	              gdb_cmd(gdb_packet_p()),
	              gdb_cmd(gdb_packet_vcont()),
	              gdb_cmd(gdb_packet_m()),
	              gdb_any());
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
