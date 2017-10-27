/*******************************************************************************
* GPIO
*******************************************************************************/
#ifndef RC_GPIO
#define RC_GPIO

#define HIGH 1
#define LOW 0

typedef enum rc_pin_direction_t{
	INPUT_PIN,
	OUTPUT_PIN
}rc_pin_direction_t;

typedef enum rc_pin_edge_t{
	EDGE_NONE,
	EDGE_RISING,
	EDGE_FALLING,
	EDGE_BOTH
}rc_pin_edge_t;

int rc_gpio_export(int pin);
int rc_gpio_unexport(int pin);
int rc_gpio_set_dir(int pin, rc_pin_direction_t dir);
int rc_gpio_set_value(int pin, int value);
int rc_gpio_get_value(int pin);
int rc_gpio_set_edge(int pin, rc_pin_edge_t edge);
int rc_gpio_fd_open(int pin);
int rc_gpio_fd_close(int fd);

int rc_gpio_init_mmap();
int rc_gpio_set_value_mmap(int pin, int state);
int rc_gpio_get_value_mmap(int pin);

#endif // RC_GPIO
