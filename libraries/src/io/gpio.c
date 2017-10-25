/*******************************************************************************
* gpio.c
*******************************************************************************/
/*******************************************************************************
* Thank you Derek Molloy for this excellent gpio code. Here is it distributed
* as a modified version for better integration into the rest of the library
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include "rc/io/gpio.h"
#include "rc/preprocessor_macros.h"
#include "gpio_registers.h"

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define MAX_BUF 64


static volatile uint32_t *map; // pointer to /dev/mem
static int mmap_initialized_flag = 0; // boolean to check if mem mapped


/*******************************************************************************
* rc_gpio_export
******************************************************************************/
int rc_gpio_export(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
	fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
	if(fd<0){
		fprintf(stderr,"ERROR: in rc_gpio_export, failed to open gpio file handle\n");
		return -1;
	}
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/*******************************************************************************
* rc_gpio_unexport
*******************************************************************************/
int rc_gpio_unexport(unsigned int gpio)
{
	int fd, len;
	char buf[MAX_BUF];
	fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
	if (fd < 0) {
		fprintf(stderr,"ERROR: in rc_gpio_unexport, failed to open gpio file handle\n");
		return -1;
	}
	len = snprintf(buf, sizeof(buf), "%d", gpio);
	write(fd, buf, len);
	close(fd);
	return 0;
}

/*******************************************************************************
* rc_gpio_set_dir
*******************************************************************************/
int rc_gpio_set_dir(int gpio, rc_pin_direction_t out_flag)
{
	int fd;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%i/direction", gpio);
	fd = open(buf, O_WRONLY);
	if(fd<0){
		fprintf(stderr,"ERROR: in rc_gpio_set_dir, failed to open gpio file handle\n");
		return -1;
	}
	if(out_flag==OUTPUT_PIN)	write(fd, "out", 4);
	else				write(fd, "in", 3);
	// close and return
	close(fd);
	return 0;
}

/*******************************************************************************
* rc_gpio_set_value
*******************************************************************************/
int rc_gpio_set_value(unsigned int gpio, int value)
{
	int fd;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
	fd = open(buf, O_WRONLY);
	if(fd<0){
		fprintf(stderr,"ERROR: in rc_gpio_set_value, failed to open gpio file handle\n");
		return -1;
	}
	if(value)	write(fd, "1", 2);
	else		write(fd, "0", 2);
	close(fd);
	return 0;
}

/*******************************************************************************
* rc_gpio_get_value
*******************************************************************************/
int rc_gpio_get_value(unsigned int gpio)
{
	int fd, ret;
	char buf[MAX_BUF];
	char ch;
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
	fd = open(buf, O_RDONLY);
	// make sure file descriptor opened correctly
	if (fd < 0) {
		fprintf(stderr,"ERROR: in rc_gpio_get_value failed to open gpio file handle\n");
		return fd;
	}
	read(fd, &ch, 1);
	if (ch != '0') ret = 1;
	else ret = 0;
	close(fd);
	return ret;
}


/*******************************************************************************
* rc_gpio_set_edge
*******************************************************************************/
int rc_gpio_set_edge(unsigned int gpio, rc_pin_edge_t edge)
{
	int fd, ret, bytes;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
	fd = open(buf, O_WRONLY);
	if (fd < 0) {
		fprintf(stderr,"ERROR: in rc_gpio_set_edge, failed to open gpio file handle\n");
		return fd;
	}
	switch(edge){
		case EDGE_NONE:
			bytes=5;
			ret=write(fd, "none", bytes);
			break;
		case EDGE_RISING:
			bytes=7;
			ret=write(fd, "rising", bytes);
			break;
		case EDGE_FALLING:
			bytes=8;
			ret=write(fd, "falling", bytes);
			break;
		case EDGE_BOTH:
			bytes=5;
			ret=write(fd, "both", bytes);
			break;
		default:
			printf("ERROR: invalid edge direction\n");
			return -1;
	}
	// make sure write worked
	if(ret!=bytes){
		fprintf(stderr,"ERROR: in rc_gpio_set_edge, failed to write to gpio file handle\n");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/*******************************************************************************
* rc_gpio_fd_open
*******************************************************************************/
int rc_gpio_fd_open(unsigned int gpio)
{
	int fd;
	char buf[MAX_BUF];
	snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
	fd = open(buf, O_RDONLY | O_NONBLOCK );
	if (fd < 0) {
		fprintf(stderr,"ERROR: in rc_gpio_fd_open, failed to open gpio file handle\n");
		return -1;
	}
	return fd;
}

/*******************************************************************************
* rc_gpio_fd_close
*******************************************************************************/
int rc_gpio_fd_close(int fd)
{
	return close(fd);
}




/*******************************************************************************
* int rc_gpio_init_mmap()
*
* by default gpio subsystems 0,1,&2 have clock signals on boot
* this function exports a pin from the last subsystem (113) so the
* gpio driver enables the clock signal so the rest of the gpio
* functions here work.
*******************************************************************************/
int rc_gpio_init_mmap()
{
	// return immediately if gpio already initialized
	if(mmap_initialized_flag){
		return 0;
	}

	int fd = open("/dev/mem", O_RDWR);
	errno=0;
	if(unlikely(fd==-1)){
		fprintf(stderr,"ERROR: in rc_init_mmap_gpio,Unable to open /dev/mem\n");
		if(unlikely(errno==EPERM)) fprintf(stderr,"Insufficient privileges\n");
		return -1;
	}
	map = (uint32_t*)mmap(NULL, MMAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,\
								fd, MMAP_OFFSET);
	if(map == MAP_FAILED) {
		close(fd);
		fprintf(stderr,"ERROR: in rc_init_mmap_gpio, Unable to map /dev/mem\n");
		return -1;
	}

	// now we must enable clock signal to the gpio subsystems
	// first open the clock register to see if the PWMSS has clock enabled
	int clock_fd;
	clock_fd = open("/dev/mem", O_RDWR);
	if(clock_fd == -1) {
		fprintf(stderr,"ERROR: in rc_init_mmap_gpio,Unable to open /dev/mem\n");
		if(unlikely(errno==EPERM)) fprintf(stderr,"Insufficient privileges\n");
		return -1;
	}
	volatile char *cm_per_base;
	cm_per_base=mmap(0,CM_PER_PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,\
								clock_fd,CM_PER);
	if(cm_per_base == (void *) -1) {
		close(fd);
		close(clock_fd);
		fprintf(stderr,"ERROR: in rc_init_mmap_gpio, Unable to map /dev/mem\n");
		return -1;
	}

	#ifdef DEBUG
	printf("turning on cm-per module_enable bits for gpio 1,2,3\n");
	#endif
	*(uint16_t*)(cm_per_base + CM_PER_GPIO1_CLKCTRL) |= MODULEMODE_ENABLE;
	*(uint16_t*)(cm_per_base + CM_PER_GPIO2_CLKCTRL) |= MODULEMODE_ENABLE;
	*(uint16_t*)(cm_per_base + CM_PER_GPIO3_CLKCTRL) |= MODULEMODE_ENABLE;

	#ifdef DEBUG
	printf("new cm_per_gpio3_clkctrl: %d\n", *(uint16_t*)(cm_per_base+CM_PER_GPIO3_CLKCTRL));
	#endif
	// done setting clocks, close map
	close(clock_fd);


	// export gpio 113 so the gpio driver brings module 3 online
	// module 1 and 2 are online already for system functions
	if(rc_gpio_export(113)){
		fprintf(stderr,"ERROR: in rc_init_mmap_gpio, failed to export pin\n");
		return -1;
	}

	mmap_initialized_flag=1;
	return 0;
}

/*******************************************************************************
* int rc_gpio_set_value_mmap(int pin, int state)
*
* write HIGH or LOW to a pin
* pinmux must already be configured for output
*******************************************************************************/
int rc_gpio_set_value_mmap(int pin, int state)
{
	if(rc_gpio_init_mmap()){
		return -1;
	}
	if(pin<0 || pin>128){
		fprintf(stderr,"ERROR: in rc_gpio_set_value_mmap(),invalid gpio pin\n");
		return -1;
	}
	int bank = pin/32;
	int id = pin - bank*32;
	int bank_offset;
	switch(bank){
		case 0:
			bank_offset=GPIO0;
			break;
		case 1:
			bank_offset=GPIO1;
			break;
		case 2:
			bank_offset=GPIO2;
			break;
		case 3:
			bank_offset=GPIO3;
			break;
		default:
			return -1;
	}
	//map[(bank_offset-MMAP_OFFSET+GPIO_OE)/4] &= ~(1<<p.bank_id);
	if(state) map[(bank_offset-MMAP_OFFSET+GPIO_DATAOUT)/4] |= (1<<id);
	else map[(bank_offset-MMAP_OFFSET+GPIO_DATAOUT)/4] &= ~(1<<id);
	return 0;
}

/*******************************************************************************
* int rc_gpio_get_value_mmap(int pin)
*
* returns 1 or 0 for HIGH or LOW
* pinMUX must already be configured for input
*******************************************************************************/
int rc_gpio_get_value_mmap(int pin)
{
	if(rc_gpio_init_mmap()){
		return -1;
	}
	if(pin<0 || pin>128){
		fprintf(stderr,"ERROR: in rc_gpio_get_value_mmap(),invalid gpio pin\n");
		return -1;
	}
	int bank = pin/32;
	int id = pin - bank*32;
	int bank_offset;
	switch(bank){
		case 0:
			bank_offset=GPIO0;
			break;
		case 1:
			bank_offset=GPIO1;
			break;
		case 2:
			bank_offset=GPIO2;
			break;
		case 3:
			bank_offset=GPIO3;
			break;
		default:
			return -1;
	}
	return (map[(bank_offset-MMAP_OFFSET+GPIO_DATAIN)/4] & (1<<id))>>id;
}