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

int rc_gpio_export(unsigned int gpio);
int rc_gpio_unexport(unsigned int gpio);
int rc_gpio_set_dir(int gpio, rc_pin_direction_t dir);
int rc_gpio_set_value(unsigned int gpio, int value);
int rc_gpio_get_value(unsigned int gpio);
int rc_gpio_set_edge(unsigned int gpio, rc_pin_edge_t edge);
int rc_gpio_fd_open(unsigned int gpio);
int rc_gpio_fd_close(int fd);

int rc_gpio_init_mmap();
int rc_gpio_set_value_mmap(int pin, int state);
int rc_gpio_get_value_mmap(int pin);

#endif // RC_GPIO
