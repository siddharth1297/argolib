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

// Remove overhead
typedef struct pool_overhead pool_overhead;
typedef struct request_box request_box;
typedef struct unit_tt unit_tt;

struct request_box {

  pthread_mutex_t lock;
  pthread_cond_t cond;

  int thief_id;
};

struct unit_tt {
  unit_tt *p_prev;
  unit_tt *p_next;
  ABT_thread thread;
};

struct pool_overhead {
  int id;
  Task_handle *mailbox;
  int counter;
  request_box *rb;
  unit_tt *p_head;
  unit_tt *p_tail;
  int wu; // work units in the pool
};

// scheduler
typedef struct {
  int event_freq;
} sched_data_tt;

/******************************************************************************/
/* Normal RAND_WS                                    */
/******************************************************************************/
typedef struct {
  int event_freq;
} sched_data_t;

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
};

#endif
