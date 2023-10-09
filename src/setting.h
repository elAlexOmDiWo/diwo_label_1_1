#pragma once

#include "main.h"

#define XSTR(x) STR(x)
#define STR(x) #x

#pragma message "Debug: " XSTR(__DEBUG__)

//#warning Debug is #__DEBUG__

#if ( __DEBUG__ != 0 )
#define WAITE_CONNECTION_TIME       100
#define WAITE_DISCONNECTION_TIME    1200
#else
#define WAITE_CONNECTION_TIME       10
#define WAITE_DISCONNECTION_TIME    120
#endif