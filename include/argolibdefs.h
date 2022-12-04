/*
 * Common definitions and includes for argolib
 */
#ifndef ARGOLIBDEFS_H
#define ARGOLIBDEFS_H

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "abt.h"

typedef ABT_thread Task_handle;
typedef void (*fork_t)(void *args);

typedef struct unit_t unit_t;

struct unit_t {
  unit_t *p_prev;
  unit_t *p_next;
  ABT_thread thread;
};

typedef struct {
  int event_freq;
} sched_data_t;

typedef struct request_box {
  pthread_mutex_t lock;
  pthread_cond_t cond;
  int thief_id;
} request_box_t;

typedef struct pool_overhead {
  int id;
  Task_handle *mailbox;
  int counter;
  request_box_t *rb;
  unit_t *p_head;
  unit_t *p_tail;
  int wu; // work units in the pool
} pool_overhead_t;

#endif
