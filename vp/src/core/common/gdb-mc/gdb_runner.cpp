#include "gdb_runner.h"

GDBServerRunner::GDBServerRunner(sc_core::sc_module_name name, GDBServer *server, debugable *hart) {
	this->hart = hart;
	this->stop_event = server->get_stop_event(hart);
	server->set_run_event(hart, &this->run_event);

	hart->enable_debug();
	SC_THREAD(run);
}

void GDBServerRunner::run(void) {
	for (;;) {
		sc_core::wait(run_event);
		hart->run();
		stop_event->notify();

		if (hart->get_status() == CoreExecStatus::Terminated)
			break;
	}
}
