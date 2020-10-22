#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <err.h>

#include "timer.h"

/* As defined in nanosleep(3) */
#define NS_MAX 999999999

/* Signal used to unblock nanosleep in thread */
#define SIGNUM SIGUSR1

static int
xnanosleep(const struct timespec *timespec) {
	if (nanosleep(timespec, NULL) == 0)
		return 0; /* success */

	if (errno == EINTR)
		return -1; /* received signal via pthread_kill, terminate */

	err(EXIT_FAILURE, "nanosleep failed"); /* EFAULT, EINVAL, … */
}

static void *
callback(void *arg)
{
	struct timespec timespec;
	Timer::Context *ctx = (Timer::Context*)arg;

	auto count = ctx->duration.count();
	while (count > NS_MAX) {
		timespec.tv_nsec = NS_MAX;
		if (xnanosleep(&timespec))
			return NULL; /* pthread_kill */
		count -= NS_MAX;
	}

	timespec.tv_nsec = count;
	if (xnanosleep(&timespec))
		return NULL; /* pthread_kill */

	ctx->fn(ctx->arg);
	return NULL;
}

Timer::Timer(Callback fn, void *arg)
  : ctx(std::chrono::nanoseconds(0), fn, arg) {
	running = false;
}

Timer::~Timer(void) {
	pause();
}

void Timer::start(std::chrono::nanoseconds ns) {
	if (running && (errno = pthread_join(thread, NULL)))
		throw std::system_error(errno, std::generic_category());

	/* Update duration for new callback */
	ctx.duration = ns;

	if ((errno = pthread_create(&thread, NULL, callback, &ctx)))
		throw std::system_error(errno, std::generic_category());
	running = true;
}

void Timer::pause(void) {
	struct sigaction sa;

	if (!running)
		return;

	sa.sa_handler = SIG_IGN;
	sa.sa_flags = SA_RESTART;
	if (sigemptyset(&sa.sa_mask) == -1)
		throw std::system_error(errno, std::generic_category());

	if (sigaction(SIGNUM, &sa, NULL) == -1)
		throw std::system_error(errno, std::generic_category());

	/* Signal is ignored in main thread, send it to background
	 * thread and restore the default handler afterwards. */
	stop_thread();

	sa.sa_handler = SIG_DFL;
	if (sigaction(SIGNUM, &sa, NULL) == -1)
		throw std::system_error(errno, std::generic_category());
}

void Timer::stop_thread(void) {
	/* Attempt to cancel thread first, for the event that it hasn't
	 * invoked nanosleep yet. The nanosleep function is a
	 * cancelation point, as per pthreads(7). If the thread is
	 * blocked in nanosleep the signal should interrupt the
	 * nanosleep system call and cause thread termination. */

	assert(running);
	pthread_cancel(thread);

	if ((errno = pthread_kill(thread, SIGNUM)))
		throw std::system_error(errno, std::generic_category());
	if ((errno = pthread_join(thread, NULL)))
		throw std::system_error(errno, std::generic_category());
	running = false;
}
