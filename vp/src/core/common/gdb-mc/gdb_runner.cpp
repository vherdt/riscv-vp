#include "gdb_runner.h"

GDBServerRunner::GDBServerRunner(sc_core::sc_module_name name, GDBServer *server, debugable *hart) {
	this->hart = hart;
	this->gdb_event = server->get_event(hart);
	server->set_event(hart, &this->run_event);

	hart->debug_mode = true;
	SC_THREAD(run);
}

void GDBServerRunner::run(void) {
	for (;;) {
		sc_core::wait(run_event);
		hart->run();
		gdb_event->notify();
	}
}
