#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "libusb-1.0.30/libusb/libusb.h"

// mouse pids
#define TARGET_VID 0x3554
#define TARGET_PID_WIRED 0xf54d
#define TARGET_PID_WIRELESS 0xf54f

// mouse pid helper
#define IS_TARGET_PID(pid) ((pid) == TARGET_PID_WIRED || (pid) == TARGET_PID_WIRELESS)

#endif
