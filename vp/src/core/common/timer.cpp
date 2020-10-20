#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "timer.h"

static void *
callback(void *arg)
{
	Timer::Context *ctx = (Timer::Context*)arg;

	// TODO: sleep

	ctx->fn(ctx->arg);
	return NULL;
}

Timer::Timer(std::chrono::microseconds usecs, Callback fn, void *arg)
  : ctx(usecs, fn, arg) {
	if ((errno = pthread_create(&thread, NULL, callback, &ctx)))
		throw std::system_error(errno, std::generic_category());
}

Timer::~Timer(void) {
	stop();
}

void Timer::stop(void) {
	if ((errno = pthread_kill(thread, SIGTERM)))
		throw std::system_error(errno, std::generic_category());
}
