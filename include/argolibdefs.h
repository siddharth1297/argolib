/*
 * Common definitions and includes for argolib
 */
#ifndef ARGOLIBDEFS_H
#define ARGOLIBDEFS_H

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

#include "abt.h"

typedef ABT_thread Task_handle;
typedef void (*fork_t)(void *args);

typedef struct unit_t unit_t;
typedef struct pool_t pool_t;

struct unit_t {
  unit_t *p_prev;
  unit_t *p_next;
  ABT_thread thread;
};

struct pool_t {
  pthread_mutex_t lock;
  unit_t *p_head;
  unit_t *p_tail;
  size_t task_count;  // No of tasks produced by this pool
  size_t task_stolen; // No of tasks stolen from this pool
};

typedef struct {
  int event_freq;
} sched_data_t;

#endif
