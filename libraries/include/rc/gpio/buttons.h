#ifndef RC_BUTTONS_H
#define RC_BUTTONS_H

/*******************************************************************************
* roboticscape_buttons.h
*
* function declarations for starting and stopping button handlers
* the rest of the button functions are in roboticscape.h for the user to access
*******************************************************************************/

int initialize_button_handlers();
int wait_for_button_handlers_to_join();

/*******************************************************************************
* BUTTONS
*
* The Robotics Cape includes two buttons labeled PAUSE and MODE. Like the LEDs,
* they are not used by any background library functions and the user can assign
* them to any function they wish. However, the user is encouraged to use the
* pause button to toggle the program flow state between PAUSED and RUNNING
* using the previously described rc_set_state() function.
*
* @ typedef enum rc_button_state_t
* 
* A button state can be either RELEASED or PRESSED as defined by this enum.
*
* @ int rc_set_pause_pressed_func(int (*func)(void))
* @ int rc_set_pause_released_func(int (*func)(void))
* @ int rc_set_mode_pressed_func(int (*func)(void))
* @ int rc_set_mode_released_func(int (*func)(void))
*
* rc_initialize() sets up interrupt handlers that run in the background to
* poll changes in button state in a way that uses minimal resources. The 
* user can assign which function should be called when either button is pressed
* or released. Functions can also be assigned under both conditions.
* for example, a timer could be started when a button is pressed and stopped
* when the button is released. Pass
*
* For simple tasks like pausing the robot, the user is encouraged to assign
* their function to be called when the button is released as this provides 
* a more natural user experience aligning with consumer product functionality.
* 
* The user can also just do a basic call to rc_get_pause_button_state() or
* rc_get_mode_buttom_state() which returns the enumerated type RELEASED or 
* PRESSED.
*
* See the rc_blink and rc_test_buttons example programs for sample use cases.
******************************************************************************/
typedef enum rc_button_state_t {
	RELEASED,
	PRESSED
} rc_button_state_t;
int rc_set_pause_pressed_func(void (*func)(void));
int rc_set_pause_released_func(void (*func)(void));
int rc_set_mode_pressed_func(void (*func)(void));
int rc_set_mode_released_func(void (*func)(void));
rc_button_state_t rc_get_pause_button();
rc_button_state_t rc_get_mode_button();

#endif // RC_BUTTONS_H
