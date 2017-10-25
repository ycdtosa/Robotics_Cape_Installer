/*******************************************************************************
* version.c
*

*******************************************************************************/

#include "rc/version.h"


/*******************************************************************************
* float rc_version_float()
*
* Returns a floating point representation of the roboticscape package
* version for comparing
*******************************************************************************/
float rc_version_float()
{
	return RC_LIB_VERSION_FLOAT;
}


/*******************************************************************************
* @ const char* rc_version_string()
*
* Returns a string of the roboticscape package version for printing.
*******************************************************************************/
const char* rc_version_string()
{
	// annoying macro hack to convert to string
	#define Q(x) #x
	#define QUOTE(x) Q(x)
	return QUOTE(RC_LIB_VERSION_STRING);
	#undef Q
	#undef QUOTE
}
