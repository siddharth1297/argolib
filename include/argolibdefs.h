/*
 * Common definitions and includes for argolib
 */
#ifndef ARGOLIBDEFS_H
#define ARGOLIBDEFS_H

#include <stdlib.h>
#include "abt.h"

typedef ABT_thread Task_handle;
typedef void (*fork_t)(void *args);

#endif
