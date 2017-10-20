#ifndef RC_VERSION_H
#define RC_VERSION_H

// library version, can also be printed from the command line with the included
// example program rc_version
#define RC_LIB_VERSION_FLOAT	0.40
#define RC_LIB_VERSION_STRING	0.4.0

float rc_version_float();
const char* rc_version_string();

#endif // RC_VERSION_H