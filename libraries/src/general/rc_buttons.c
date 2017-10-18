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
static void (*pause_press_func)(void)		= &rc_null_func;
static void (*pause_release_func)(void)		= &rc_null_func;
static void (*mode_press_func)(void)		= &rc_null_func;
static void (*mode_release_func)(void)		= &rc_null_func;

// mutex to allow blocking on button press
static pthread_mutex_t pause_press_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t pause_release_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mode_press_mutex		= PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mode_release_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pause_press_condition	= PTHREAD_COND_INITIALIZER;
static pthread_cond_t pause_release_condition	= PTHREAD_COND_INITIALIZER;
static pthread_cond_t mode_press_condition	= PTHREAD_COND_INITIALIZER;
static pthread_cond_t mode_release_condition	= PTHREAD_COND_INITIALIZER;


// local thread function declarations
static void* pause_press_handler(void* ptr);
static void* pause_release_handler(void* ptr);
static void* mode_press_handler(void* ptr);
static void* mode_release_handler(void* ptr);

// local thread structs
static pthread_t pause_press_thread;
static pthread_t pause_release_thread;
static pthread_t mode_press_thread;
static pthread_t mode_release_thread;

// state variables
static int initialized_flag = 0;	// set to 1 but initialize_buttons
static int shutdown_flag = 0;		// set to 1 by rc_stop_buttons

// modes for the watch threads to run in
typedef enum thread_mode_t {
	PAUSE_PRESS,
	PAUSE_RELEASE,
	MODE_PRESS,
	MODE_RELEASE
} thread_mode_t;


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
	thread_mode_t mode;
	// basic gpio setup
	if(rc_gpio_export(PAUSE_BTN)){
		fprintf(stderr,"ERROR: Failed to export gpio pin %d\n", PAUSE_BTN);
		return -1;
	}
	if(rc_gpio_set_dir(PAUSE_BTN, INPUT_PIN)){
		fprintf(stderr,"ERROR: Failed to set gpio pin %d as output\n", PAUSE_BTN);
		return -1;
	}
	if(rc_gpio_export(MODE_BTN)){
		fprintf(stderr,"ERROR: Failed to export gpio pin %d\n", MODE_BTN);
		return -1;
	}
	if(rc_gpio_set_dir(MODE_BTN, INPUT_PIN)){
		fprintf(stderr,"ERROR: Failed to set gpio pin %d as output\n", MODE_BTN);
		return -1;
	}
	rc_gpio_set_edge(MODE_BTN, EDGE_BOTH);
	rc_gpio_set_edge(PAUSE_BTN, EDGE_BOTH);

	// wipe the callback functions
	#ifdef DEBUG
	printf("setting defualt callback function\n");
	#endif
	rc_set_button_callback(PAUSE,PRESSED,&rc_null_func);
	rc_set_button_callback(PAUSE,RELEASED,&rc_null_func);
	rc_set_button_callback(MODE,PRESSED,&rc_null_func);
	rc_set_button_callback(MODE,RELEASED,&rc_null_func);

	#ifdef DEBUG
	printf("starting button threads\n");
	#endif
	mode=PAUSE_PRESS;
	pthread_create(&pause_press_thread, NULL, button_handler,(void*)mode);
	mode=PAUSE_release;
	pthread_create(&pause_release_thread, NULL, button_handler,(void*)mode);
	mode=MODE_PRESS;
	pthread_create(&mode_press_thread, NULL, button_handler,(void*)mode);
	mode=MODE_RELEASE;
	pthread_create(&mode_release_thread, NULL, button_handler,(void*)mode);

	// apply priority to all threads
	#ifdef DEBUG
	printf("setting button thread priorities\n");
	#endif
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO)-5;
	pthread_setschedparam(pause_press_thread, SCHED_FIFO, &params);
	pthread_setschedparam(pause_release_thread, SCHED_FIFO, &params);
	pthread_setschedparam(mode_press_thread, SCHED_FIFO, &params);
	pthread_setschedparam(mode_release_thread, SCHED_FIFO, &params);

	return 0;
}

/*******************************************************************************
* void* pause_press_handler(__unused void* ptr)
*
* wait on falling edge of pause button
*******************************************************************************/
void* button_handler(void* ptr)
{
	thread_mode_t mode = (thread_mode_t)ptr;
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
			if(rc_get_button_state(PAUSE)==PRESSED){
				rc_usleep(500);
				if(rc_get_button_state(PAUSE)==PRESSED){
					rc_usleep(500);
					if(rc_get_button_state(PAUSE)==PRESSED){
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
* rc_button_state_t rc_get_button_state(rc_button_t button)
*******************************************************************************/
rc_button_state_t rc_get_button_state(rc_button_t button);
{
	if(!buttons_en){
		fprintf(stderr,"ERROR: call to rc_get_button_state() when buttons have not been initialized.\n");
		return -2;
	}
	switch(button){
	case PAUSE:
		if(rc_gpio_get_value_mmap(PAUSE_BTN)==HIGH) return RELEASED;
		else return PRESSED;
	case MODE:
		if(rc_gpio_get_value_mmap(MODE_BTN)==HIGH) return RELEASED;
		else return PRESSED;
	default:
		fprintf(stderr,"ERROR: in rc_get_button_state, invalid button enum\n");
		return RELEASED;
	}
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
int wait_for_button_cleanup()
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








/*******************************************************************************
* DEPRECATED button function assignments
*******************************************************************************/
int rc_set_pause_pressed_func(void (*func)(void))
{
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to paused_press_func\n");
		return -1;
	}
	pause_press_func = func;
	return 0;
}
int rc_set_pause_released_func(void (*func)(void)){
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to paused_release_func\n");
		return -1;
	}
	pause_release_func = func;
	return 0;
}
int rc_set_mode_pressed_func(void (*func)(void)){
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to mode_press_func\n");
		return -1;
	}
	mode_press_func = func;
	return 0;
}
int rc_set_mode_released_func(void (*func)(void)){
	if(func==NULL){
		printf("ERROR: trying to assign NULL pointer to mode_release_func\n");
		return -1;
	}
	mode_release_func = func;
	return 0;
}

/*******************************************************************************
* DEPRECATED
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
* DEPRECATED
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