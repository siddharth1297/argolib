/*
 * Common definitions and includes for argolib
 */
//#define _GNU_SOURCE // NOT WORING FOR CPP
#ifndef ARGOLIBDEFS_H
#define ARGOLIBDEFS_H

#include "cpucounters.h" //PCM related: https://github.com/intel/pcm
#include "utils.h"       //PCM related: https://github.com/intel/pcm
#include <assert.h>
#include <chrono>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

typedef struct pool_energy {

  pthread_mutex_t lock;
  pthread_mutex_t active_lock;
  pthread_cond_t cond;
  unit_t *p_head;
  unit_t *p_tail;
  int active;
  size_t task_count;
  size_t task_stolen;
} pool_energy_t;

#endif
