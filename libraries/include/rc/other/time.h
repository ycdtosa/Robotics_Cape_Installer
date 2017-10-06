/*******************************************************************************
* time
*
* @ void rc_nanosleep(uint64_t ns)
*
* A wrapper for the normal UNIX nanosleep function which takes a number of
* nanoseconds instead of a timeval struct. This also handles restarting
* nanosleep with the remaining time in the event that nanosleep is interrupted
* by a signal. There is no upper limit on the time requested.
*
* @ void rc_usleep(uint64_t ns)
*
* The traditional usleep function, however common, is deprecated in linux as it
* uses SIGALARM which interferes with alarm and timer functions. This uses the
* new POSIX standard nanosleep to accomplish the same thing which further
* supports sleeping for lengths longer than 1 second. This also handles
* restarting nanosleep with the remaining time in the event that nanosleep is
* interrupted by a signal. There is no upper limit on the time requested.
*
* @ uint64_t rc_timespec_to_micros(timespec ts)
*
* Returns a number of microseconds corresponding to a timespec struct.
* Useful because timespec structs are annoying.
*
* @ uint64_t rc_timespec_to_millis(timespec ts)
*
* Returns a number of milliseconds corresponding to a timespec struct.
* Useful because timespec structs are annoying.
*
* @ uint64_t rc_timeval_to_micros(timeval tv)
*
* Returns a number of microseconds corresponding to a timespec struct.
* Useful because timeval structs are annoying.
*
* @ uint64_t rc_timeval_to_millis(timeval ts)
*
* Returns a number of milliseconds corresponding to a timespec struct.
* Useful because timeval structs are annoying.
*
* @ uint64_t rc_nanos_since_epoch()
*
* Returns the number of nanoseconds since epoch using system CLOCK_REALTIME
* This function itself takes about 1100ns to complete at 1ghz under ideal
* circumstances.
*
* @ uint64_t rc_nanos_since_boot()
*
* Returns the number of nanoseconds since system boot using CLOCK_MONOTONIC
* This function itself takes about 1100ns to complete at 1ghz under ideal
* circumstances.
*
* @ uint64_t rc_nanos_thread_time()
*
* Returns the number of nanoseconds from when when the calling thread was
* started in CPU time. This time only increments when the processor is working
* on the calling thread and not when the thread is sleeping. This is usually for
* timing how long blocks of user-code take to execute. This function itself
* takes about 2100ns to complete at 1ghz under ideal circumstances.
*
* @ timespec rc_timespec_diff(timespec start, timespec end)
*
* Returns the time difference between two timespec structs as another timespec.
* Convenient for use with nanosleep() function and accurately timed loops.
* Unlike timespec_sub defined in time.h, rc_timespec_diff does not care which
* came first, A or B. A positive difference in time is always returned.
*
* @ int rc_timespec_add(timespec* start, double seconds)
*
* Adds an amount of time in seconds to a timespec struct. The time added is a
* floating point value to make respresenting fractions of a second easier.
* the timespec is passed as a pointer so it can be modified in place.
* Seconds may be negative.
*******************************************************************************/
#ifndef RC_TIME_H
#define RC_TIME_H

#include <time.h>	// for timespec
#include <sys/time.h>	// for timeval

typedef struct timespec	timespec;
typedef struct timeval timeval;

void rc_nanosleep(uint64_t ns);
void rc_usleep(unsigned int us);
uint64_t rc_timespec_to_micros(timespec ts);
uint64_t rc_timespec_to_millis(timespec ts);
uint64_t rc_timeval_to_micros(timeval tv);
uint64_t rc_timeval_to_millis(timeval tv);
uint64_t rc_nanos_since_epoch();
uint64_t rc_nanos_since_boot();
uint64_t rc_nanos_thread_time();
timespec rc_timespec_diff(timespec A, timespec B);
void rc_timespec_add(timespec* start, double seconds);

#endif // RC_TIME_H
