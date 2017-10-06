/*******************************************************************************
* rc-usefulincludes.h
*
* This is a collection of the most common system headers
* This may be used by the user as an "uber-include" in their projects to clean
* up what would otherwise be a cluttered list of includes at the top of their
* own program.
*
* We recommended including this BEFORE roboticscape.h to make sure _GNU_SOURCE
* is the first include, otherwise macros may not work.
*
* James Strawson - 2017
*******************************************************************************/

#ifndef RC_USEFUL_INCLUDES
#define RC_USEFUL_INCLUDES

#define _GNU_SOURCE // to enable macros in pthread

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>	// file control
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>	// usleep, nanosleep
#include <math.h>	// atan2 and fabs
#include <pthread.h>	// multi-threading


/*******************************************************************************
* Useful Constants
*******************************************************************************/
#define DEG_TO_RAD		0.0174532925199
#define RAD_TO_DEG		57.295779513
#define PI			M_PI
#define TWO_PI			(2.0 * M_PI)

/*******************************************************************************
* Useful Macros
*******************************************************************************/
#define ARRAY_SIZE(array) sizeof(array)/sizeof(array[0])
#define min(a, b) 	((a < b) ? a : b)

#endif // RC_USEFUL_INCLUDES