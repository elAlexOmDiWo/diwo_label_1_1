#pragma once

#include "main.h"

void *get_connect(void);

#define XSTR(x) STR(x)
#define STR(x) #x

#pragma message "Debug: " XSTR(__DEBUG__)

//#warning Debug is #__DEBUG__

