#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <err.h>

#include "timer.h"

/* As defined in nanosleep(3) */
#define NS_MAX 999999999

static void
xnanosleep(const struct timespec *timespec) {
	if (nanosleep(timespec, NULL)) /* no SA_RESTART */
		err(EXIT_FAILURE, "nanosleep failed");
}

static void *
callback(void *arg)
{
	struct timespec timespec;
	Timer::Context *ctx = (Timer::Context*)arg;

	auto count = ctx->duration.count();
	while (count > NS_MAX) {
		timespec.tv_nsec = NS_MAX;
		xnanosleep(&timespec);
		count -= NS_MAX;
	}

	timespec.tv_nsec = count;
	xnanosleep(&timespec);

	/* Add explicit cancelation point before callback invocation */
	pthread_testcancel();

	ctx->fn(ctx->arg);
	return NULL;
}

Timer::Timer(std::chrono::nanoseconds ns, Callback fn, void *arg)
  : ctx(ns, fn, arg) {
	if ((errno = pthread_create(&thread, NULL, callback, &ctx)))
		throw std::system_error(errno, std::generic_category());
}

Timer::~Timer(void) {
	stop();
}

void Timer::stop(void) {
	/* We don't kill the thread directly, instead we cancel it which
	 * probably causes a sleeping thread to continue execution until
	 * a cancelation point is reached, however, a sleeping thread
	 * shouldn't consume much resources thus this should be ok for now. */

	if (!pthread_cancel(thread)) {
		if ((errno = pthread_join(thread, NULL)))
			throw std::system_error(errno, std::generic_category());
	}
}
