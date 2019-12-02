#define gdbf_fold(I, TYPE, FUNC)                                               \
	static mpc_val_t *gdbf_packet_##I(int n, mpc_val_t **xs)               \
	{                                                                      \
		int i, j;                                                      \
		gdb_command_t *cmd;                                            \
                                                                               \
		/* Remove optional mpc_maybe() arguments */                    \
		for (j = n - 1; j >= 0; j--)                                   \
			if (!xs[j])                                            \
				--n;                                           \
                                                                               \
		assert(n > 0);                                                 \
		cmd = gdb_new_cmd(xs[0], TYPE);                                \
		FUNC;                                                          \
                                                                               \
		return cmd;                                                    \
	}

#define GDBF_ARG_HCMD                                                          \
	do {                                                                   \
		assert(n == 3);                                                \
		                                                               \
		cmd->v.hcmd.op = *((char*)xs[1]);                              \
		cmd->v.hcmd.id = *((gdb_thread_t*)xs[2]);                      \
                                                                               \
		free(xs[1]);                                                   \
		free(xs[2]);                                                   \
	} while (0)

#define GDBF_ARG_INT                                                           \
	do {                                                                   \
		assert(n == 2);                                                \
		                                                               \
		cmd->v.ival = *((int*)xs[1]);                                  \
		free(xs[1]);                                                   \
	} while (0)

#define GDBF_ARG_VCONT                                                         \
	do {                                                                   \
		assert(n == 2);                                                \
		cmd->v.vval = (gdb_vcont_t *)xs[1];                            \
	} while (0)

#define GDBF_ARG_MEMORY                                                        \
	do {                                                                   \
		assert(n == 4);                                                \
		cmd->v.mem.addr = (size_t)*((unsigned long long*)xs[1]);       \
		cmd->v.mem.length = (size_t)*((unsigned long long *)xs[3]);    \
		free(xs[2]);                                                   \
	} while (0)

#define GDBF_ARG_BREAK                                                         \
	do {                                                                   \
		assert(n == 3);                                                \
		cmd->v.bval.type = *(int *)(xs[1]);                            \
		cmd->v.bval.address = (size_t)*((unsigned long long*)xs[2]);   \
		cmd->v.bval.kind = *(int *)(xs[3]);                            \
		free(xs[1]);                                                   \
		free(xs[2]);                                                   \
		free(xs[3]);                                                   \
	} while (0)

#define GDBF_ARG_THREAD                                                        \
	do {                                                                   \
		assert(n == 2);                                                \
		cmd->v.tval = *((gdb_thread_t *)xs[1]);                        \
		free(xs[1]);                                                   \
	} while (0)
