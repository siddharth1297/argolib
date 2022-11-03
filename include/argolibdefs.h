/*
 * Common definitions and includes for argolib
 */
#ifndef ARGOLIBDEFS_H
#define ARGOLIBDEFS_H

#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "abt.h"

typedef ABT_thread Task_handle;
typedef void (*fork_t)(void *args);

typedef long counter_t;
#define COUNTER_MIN LONG_MIN
#define COUNTER_MAX LONG_MAX

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

typedef struct tracereplay_unit_t tracereplay_unit_t;

struct tracereplay_unit_t {
  tracereplay_unit_t *p_prev;
  tracereplay_unit_t *p_next;
  ABT_thread thread;
};

typedef struct trace_data_t {
  counter_t task_id;
  int create_pool_idx;
  int exec_pool_idx;
  counter_t steal_count;
} trace_data_t;

typedef struct trace_task_list_t {
  trace_data_t trace_data;
  struct trace_task_list_t *prev;
  struct trace_task_list_t *next;
} trace_task_list_t;

typedef struct tracereplay_pool_t {
  pthread_mutex_t lock;
  tracereplay_unit_t *p_head;
  tracereplay_unit_t *p_tail;
  trace_task_list_t *trace_task_list;
} tracereplay_pool_t;

#endif
