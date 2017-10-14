/*******************************************************************************
* rc_buttons.c
*
* 4 threads for managing the pause and press buttons
*******************************************************************************/
#define _GNU_SOURCE
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "rc/general/buttons.h"
#include "rc/io/gpio.h"
#include "rc/other/preprocessor_macros.h"
#include "rc/other/time.h"
#include "rc/bbb/rc_defs.h"
#include "rc/general/state.h"

#define POLL_BUF_LEN 1024
#define POLL_TIMEOUT 100 /* 0.1 seconds */

// placeholder for button functions
// this is also in other/other.h but is so small we redefine it here to reduce
// the number of dependencies
static void rc_null_func();

// function pointers for button handlers
static void (*pause_pressed_func)(void)		= &rc_null_func;
static void (*pause_released_func)(void)	= &rc_null_func;
static void (*mode_pressed_func)(void)		= &rc_null_func;
static void (*mode_released_func)(void)		= &rc_null_func;

// mutex to allow blocking on button press
static pthread_mutex_t pause_press_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pause_releas_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mode_press_mutex		= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mode_releas_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pause_press_condition	= PTHREAD_COND_INITIALIZER;
static pthread_cond_t pause_releas_condition	= PTHREAD_COND_INITIALIZER;
static pthread_cond_t mode_press_condition	= PTHREAD_COND_INITIALIZER;
static pthread_cond_t mode_releas_condition	= PTHREAD_COND_INITIALIZER;


// local thread function declarations
static void* pause_pressed_handler(void* ptr);
static void* pause_released_handler(void* ptr);
static void* mode_pressed_handler(void* ptr);
static void* mode_released_handler(void* ptr);

// local thread structs
static pthread_t pause_pressed_thread;
static pthread_t pause_released_thread;
static pthread_t mode_pressed_thread;
static pthread_t mode_released_thread;

// state variables
static int initialized_flag = 0;	// set to 1 but initialize_buttons
static int shutdown_flag = 0;		// set to 1 by rc_stop_buttons


/*******************************************************************************
* @ void rc_null_func()
*
* A simple function that just returns. This exists so function pointers can be
* set to do nothing such as button and imu interrupt handlers.
*******************************************************************************/
static void rc_null_func(){
	return;
}

/*******************************************************************************
* int initialize_buttons()
*
* start 4 threads to handle 4 interrupt routines for pressing and
* releasing the two buttons.
*******************************************************************************/
int initialize_buttons()
{
	#ifdef DEBUG
	printf("setting pause_pressed function\n");
	#endif
	rc_set_pause_pressed_func(&rc_null_func);

	#ifdef DEBUG
	printf("setting remaining button functions\n");
	#endif
	rc_set_pause_released_func(&rc_null_func);
	rc_set_mode_pressed_func(&rc_null_func);
	rc_set_mode_released_func(&rc_null_func);

	#ifdef DEBUG
	printf("starting button threads\n");
	#endif
	pthread_create(&pause_pressed_thread, NULL, pause_pressed_handler,\
								(void*) NULL);
	pthread_create(&pause_released_thread, NULL, pause_released_handler,\
								(void*) NULL);
	pthread_create(&mode_pressed_thread, NULL, mode_pressed_handler,\
								(void*) NULL);
	pthread_create(&mode_released_thread, NULL, mode_released_handler,\
								(void*) NULL);

	// apply priority to all threads
	#ifdef DEBUG
	printf("setting button thread priorities\n");
	#endif
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO)-5;
	pthread_setschedparam(pause_pressed_thread, SCHED_FIFO, &params);
	pthread_setschedparam(pause_released_thread, SCHED_FIFO, &params);
	pthread_setschedparam(mode_pressed_thread, SCHED_FIFO, &params);
	pthread_setschedparam(mode_released_thread, SCHED_FIFO, &params);

	return 0;
}

/*******************************************************************************
*	void* pause_pressed_handler(__unused void* ptr)
*
*	wait on falling edge of pause button
*******************************************************************************/
void* pause_pressed_handler(__unused void* ptr)
{
	struct pollfd fdset[1];
	char buf[POLL_BUF_LEN];
	int gpio_fd = rc_gpio_fd_open(PAUSE_BTN);
	fdset[0].fd = gpio_fd;
	fdset[0].events = POLLPRI; // high-priority interrupt
	// keep running until the program closes
	while(rc_get_state() != EXITING) {
		// system hangs here until FIFO interrupt
		poll(fdset, 1, POLL_TIMEOUT);
		if (fdset[0].revents & POLLPRI) {
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
			// delay debouce
			rc_usleep(500);
			if(rc_get_pause_button()==PRESSED){
				rc_usleep(500);
				if(rc_get_pause_button()==PRESSED){
					rc_usleep(500);
					if(rc_get_pause_button()==PRESSED){
						pause_pressed_func();
					}
				}
			}
			// purge any interrupts that may have stacked up
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
		}
	}
	rc_gpio_fd_close(gpio_fd);
	return 0;
}

/*******************************************************************************
* @ void* pause_released_handler(__unused void* ptr)
*
* wait on rising edge of pause button
*******************************************************************************/
void* pause_released_handler(__unused void* ptr)
{
	struct pollfd fdset[1];
	char buf[POLL_BUF_LEN];
	int gpio_fd = rc_gpio_fd_open(PAUSE_BTN);
	fdset[0].fd = gpio_fd;
	fdset[0].events = POLLPRI; // high-priority interrupt
	// keep running until the program closes
	while(rc_get_state() != EXITING) {
		// system hangs here until FIFO interrupt
		poll(fdset, 1, POLL_TIMEOUT);
		if (fdset[0].revents & POLLPRI) {
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
			// delay debouce
			rc_usleep(500);
			if(rc_get_pause_button()==RELEASED){
				rc_usleep(500);
				if(rc_get_pause_button()==RELEASED){
					rc_usleep(500);
					if(rc_get_pause_button()==RELEASED){
						pause_released_func();
					}
				}
			}
			// purge any interrupts that may have stacked up
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
		}
	}
	rc_gpio_fd_close(gpio_fd);
	return 0;
}

/*******************************************************************************
*	void* mode_pressed_handler(__unused void* ptr)
*	wait on falling edge of mode button
*******************************************************************************/
void* mode_pressed_handler(__unused void* ptr)
{
	struct pollfd fdset[1];
	char buf[POLL_BUF_LEN];
	int gpio_fd = rc_gpio_fd_open(MODE_BTN);
	fdset[0].fd = gpio_fd;
	fdset[0].events = POLLPRI; // high-priority interrupt
	// keep running until the program closes
	while(rc_get_state() != EXITING) {
		// system hangs here until FIFO interrupt
		poll(fdset, 1, POLL_TIMEOUT);
		if (fdset[0].revents & POLLPRI) {
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
			// delay debouce
			rc_usleep(500);
			if(rc_get_mode_button()==PRESSED){
				rc_usleep(500);
				if(rc_get_mode_button()==PRESSED){
					rc_usleep(500);
					if(rc_get_mode_button()==PRESSED){
						mode_pressed_func();
					}
				}
			}
			// purge any interrupts that may have stacked up
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
		}
	}
	rc_gpio_fd_close(gpio_fd);
	return 0;
}

/*******************************************************************************
*	void* mode_released_handler(__unused void* ptr)
*	wait on rising edge of mode button
*******************************************************************************/
void* mode_released_handler(__unused void* ptr)
{
	struct pollfd fdset[1];
	char buf[POLL_BUF_LEN];
	int gpio_fd = rc_gpio_fd_open(MODE_BTN);
	fdset[0].fd = gpio_fd;
	fdset[0].events = POLLPRI; // high-priority interrupt
	// keep running until the program closes
	while(rc_get_state() != EXITING) {
		// system hangs here until FIFO interrupt
		poll(fdset, 1, POLL_TIMEOUT);
		if (fdset[0].revents & POLLPRI) {
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
			// delay debouce
			rc_usleep(500);
			if(rc_get_mode_button()==RELEASED){
				rc_usleep(500);
				if(rc_get_mode_button()==RELEASED){
					rc_usleep(500);
					if(rc_get_mode_button()==RELEASED){
						mode_released_func();
					}
				}
			}
			// purge any interrupts that may have stacked up
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
		}
	}
	rc_gpio_fd_close(gpio_fd);
	return 0;
}

/*******************************************************************************
*	button function assignments
*******************************************************************************/
int rc_set_pause_pressed_func(void (*func)(void))
{
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to paused_pressed_func\n");
		return -1;
	}
	pause_pressed_func = func;
	return 0;
}
int rc_set_pause_released_func(void (*func)(void)){
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to paused_released_func\n");
		return -1;
	}
	pause_released_func = func;
	return 0;
}
int rc_set_mode_pressed_func(void (*func)(void)){
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to mode_pressed_func\n");
		return -1;
	}
	mode_pressed_func = func;
	return 0;
}
int rc_set_mode_released_func(void (*func)(void)){
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to mode_released_func\n");
		return -1;
	}
	mode_released_func = func;
	return 0;
}

/*******************************************************************************
* waiting functions
*******************************************************************************/
int rc_wait_for_pause_press()
{
	if(shutdown_button_threads!=0){
		fprintf(stderr,"ERROR: call to rc_wait_for_pause_press() after buttons have been powered off.\n");
		return -1;
	}
	if(!buttons_en){
		fprintf(stderr,"ERROR: call to rc_wait_for_pause_press() when buttons have not been initialized.\n");
		return -2;
	}
	// lock mutex briefly then wait for condition signal which unlocks mutex
	pthread_mutex_lock(&pause_press_mutex);
	pthread_cond_wait(&pause_press_condition, &pause_press_mutex);
	pthread_mutex_unlock(&pause_press_mutex);
	return 0;
}

/*******************************************************************************
* waiting functions
*******************************************************************************/
int rc_wait_for_mode_press()
{
	if(shutdown_button_threads!=0){
		fprintf(stderr,"ERROR: call to rc_wait_for_mode_press() after buttons have been powered off.\n");
		return -1;
	}
	if(!buttons_en){
		fprintf(stderr,"ERROR: call to rc_wait_for_mode_press() when buttons have not been initialized.\n");
		return -2;
	}
	// lock mutex briefly then wait for condition signal which unlocks mutex
	pthread_mutex_lock(&mode_press_mutex);
	pthread_cond_wait(&mode_press_condition, &mode_press_mutex);
	pthread_mutex_unlock(&mode_press_mutex);
	return 0;
}

/*******************************************************************************
* rc_button_state_t rc_get_pause_button()
*******************************************************************************/
rc_button_state_t rc_get_pause_button()
{
	if(!buttons_en){
		fprintf(stderr,"ERROR: call to rc_get_pause_button() when buttons have not been initialized.\n");
		return -2;
	}
	if(rc_gpio_get_value_mmap(PAUSE_BTN)==HIGH){
		return RELEASED;
	}
	return PRESSED;
}

/*******************************************************************************
* rc_button_state_t rc_get_mode_button()
*******************************************************************************/
rc_button_state_t rc_get_mode_button()
{
	if(!buttons_en){
		fprintf(stderr,"ERROR: call to rc_get_mode_button() when buttons have not been initialized.\n");
		return -2;
	}
	if(rc_gpio_get_value_mmap(MODE_BTN)==HIGH){
		return RELEASED;
	}
	return PRESSED;
}


/*******************************************************************************
* void rc_stop_buttons()
*
* This tells the button handler threads to stop. It will return immediately.
*******************************************************************************/
void rc_stop_buttons(){
	shutdown_button_threads = 1;
	return;
}


/*******************************************************************************
* int wait_for_button_handlers_to_join()
*******************************************************************************/
int wait_for_button_handlers_to_join()
{
	int ret = 0;
	//allow up to 3 seconds for thread cleanup
	struct timespec thread_timeout;
	clock_gettime(CLOCK_REALTIME, &thread_timeout);
	thread_timeout.tv_sec += 3;
	int thread_err = 0;
	thread_err = pthread_timedjoin_np(pause_pressed_thread, NULL, \
							&thread_timeout);
	if(thread_err == ETIMEDOUT){
		printf("WARNING: pause_pressed_thread exit timeout\n");
		ret = -1;
	}
	thread_err = 0;
	thread_err = pthread_timedjoin_np(pause_released_thread, NULL, \
							&thread_timeout);
	if(thread_err == ETIMEDOUT){
		printf("WARNING: pause_released_thread exit timeout\n");
		ret = -1;
	}
	thread_err = 0;
	thread_err = pthread_timedjoin_np(mode_pressed_thread, NULL, \
							&thread_timeout);
	if(thread_err == ETIMEDOUT){
		printf("WARNING: mode_pressed_thread exit timeout\n");
		ret = -1;
	}
	thread_err = 0;
	thread_err = pthread_timedjoin_np(mode_released_thread, NULL, \
							&thread_timeout);
	if(thread_err == ETIMEDOUT){
		printf("WARNING: mode_released_thread exit timeout\n");
		ret = -1;
	}
	return ret;
}
