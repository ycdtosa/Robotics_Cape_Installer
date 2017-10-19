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

#define POLL_BUF_LEN	1024
#define POLL_TIMEOUT	100 // 0.1 seconds
#define NUM_THREADS	4

// local function declarations
static int get_index(rc_button_t button, rc_button_state_t state);
static void* button_handler(void* ptr);
static void rc_null_func();

// local variables
static void (*callbacks[4])(void);
static pthread_t threads[4];
static pthread_mutex_t mutex[4];
static pthread_cond_t condition[4];
static int initialized_flag = 0;	// set to 1 but initialize_buttons
static int shutdown_flag = 0;		// set to 1 by rc_stop_buttons

// modes for the watch threads to run in
typedef struct thread_mode_t {
	rc_button_t button;
	rc_button_state_t state;
	int gpio;
	int index;
} thread_mode_t;

static const thread_mode_t mode[] = {
	{PAUSE, PRESSED,  PAUSE_BTN, 0},
	{PAUSE, RELEASED, PAUSE_BTN, 1},
	{MODE,  PRESSED,  MODE_BTN,  2},
	{MODE,  RELEASED, MODE_BTN,  3}
};

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
* int rc_initialize_buttons()
*
* start 4 threads to handle 4 interrupt routines for pressing and
* releasing the two buttons.
*******************************************************************************/
int rc_initialize_buttons()
{
	int i;
	// sanity checks
	if(initialized_flag!=0 && shutdown_flag==0){
		fprintf(stderr,"ERROR: in rc_initialize_buttons, buttons already initialized\n");
		return -1;
	}
	if(initialized_flag!=0 && shutdown_flag!=0){
		fprintf(stderr,"ERROR: in rc_initialize_buttons, button threads are still shutting down\n");
		return -1;
	}
	
	
	
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
	for(i=0;i<4;i++){ 
		callbacks[i]=&rc_null_func; 
	}
	
	// reset mutex
	#ifdef DEBUG
	printf("resetting button mutex\n");
	#endif
	for(i=0;i<4;i++){ 
		mutex[i]=PTHREAD_MUTEX_INITIALIZER;
		condition[i]=PTHREAD_COND_INITIALIZER;
	}
	
	// spawn off threads
	#ifdef DEBUG
	printf("starting button threads\n");
	#endif
	for(i=0;i<4;i++){
		pthread_create(&threads[i],NULL,button_handler,(void*)&mode[i]);
	}

	// apply priority to all threads
	#ifdef DEBUG
	printf("setting button thread priorities\n");
	#endif
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO)-5;
	for(i=0;i<4;i++){
		pthread_setschedparam(threads[i], SCHED_FIFO, &params);
	}

	return 0;
}

/*******************************************************************************
* void* button_handler(void* ptr)
*
* wait on falling edge of pause button
*******************************************************************************/
void* button_handler(void* ptr)
{
	thread_mode_t* mode = (thread_mode_t*)ptr;
	struct pollfd fdset[1];
	char buf[POLL_BUF_LEN];
	int gpio_fd = rc_gpio_fd_open(mode->gpio);
	if(gpio_fd == -1){
		fprintf(stderr,"ERROR initializing buttons, failed to get GPIO fd for pin %d\n",mode->gpio);
		return;
	}
	fdset[0].fd = gpio_fd;
	fdset[0].events = POLLPRI; // high-priority interrupt
	// keep running until the program closes
	while(shutdown_flag==0) {
		// system waits here until FIFO interrupt
		poll(fdset, 1, POLL_TIMEOUT);
		if (fdset[0].revents & POLLPRI) {
			lseek(fdset[0].fd, 0, SEEK_SET);
			read(fdset[0].fd, buf, POLL_BUF_LEN);
			// delay debouce
			rc_usleep(500);
			if(rc_get_button_state(mode->button)==mode->state){
				rc_usleep(500);
				if(rc_get_button_state(mode->button)==mode->state){
					rc_usleep(500);
					if(rc_get_button_state(mode->button)==mode->state){
						callbacks(mode->index);
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
int rc_wait_for_button(rc_button_t button, rc_button_state_t state)
{
	if(shutdown_button_threads!=0){
		fprintf(stderr,"ERROR: call to rc_wait_for_button() after buttons have been powered off.\n");
		return -1;
	}
	if(!buttons_en){
		fprintf(stderr,"ERROR: call to rc_wait_for_button() when buttons have not been initialized.\n");
		return -1;
	}
	
	// lock mutex briefly then wait for condition signal which unlocks mutex
	pthread_mutex_lock(&pause_press_mutex);
	pthread_cond_wait(&pause_press_condition, &pause_press_mutex);
	pthread_mutex_unlock(&pause_press_mutex);
	return 0;
}


/*******************************************************************************
* int get_index(rc_button_t button, rc_button_state_t state)
*******************************************************************************/
int get_index(rc_button_t button, rc_button_state_t state){
	if	(button==PAUSE && state==PRESSED)	return 0;
	else if	(button==PAUSE && state==RELEASED)	return 1;
	else if	(button==MODE  && state==PRESSED)	return 2;
	else if	(button==MODE  && state==RELEASED)	return 3;
	else{
		fprintf(stderr,"ERROR in rc_buttons.c, invalid button/state combo\n");
		return -1;
	}
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
