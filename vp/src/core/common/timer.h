#ifndef RISCV_VP_TIMER_H
#define RISCV_VP_TIMER_H

#include <chrono>
#include <system_error>

#include <pthread.h>

class Timer {
public:
	typedef void (*Callback) (void*);
	class Context {
	public:
		std::chrono::nanoseconds duration;
		Callback fn;
		void *arg;

		Context(std::chrono::nanoseconds _duration, Callback _fn, void *_arg)
			: duration(_duration), fn(_fn), arg(_arg) {};
	};

	Timer(std::chrono::nanoseconds ns, Callback fn, void *arg);
	~Timer(void);

	void stop(void);

private:
	Context ctx;
	pthread_t thread;
};

#endif
