/*
 * Common definitions and includes for argolib
 */
//#define _GNU_SOURCE // NOT WORING FOR CPP
#ifndef ARGOLIBDEFS_H
#define ARGOLIBDEFS_H
    

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpucounters.h" //PCM related: https://github.com/intel/pcm
#include "utils.h"       //PCM related: https://github.com/intel/pcm
         
      

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

struct {
        int (*pcm_c_build_core_event)(uint8_t id, const char * argv);
        int (*pcm_c_init)();
        void (*pcm_c_start)();
        void (*pcm_c_stop)();
        uint64_t (*pcm_c_get_cycles)(uint32_t core_id);
        uint64_t (*pcm_c_get_instr)(uint32_t core_id);
        uint64_t (*pcm_c_get_core_event)(uint32_t core_id, uint32_t event_id);
} PCM;

#ifndef PCM_DYNAMIC_LIB
/* Library functions declaration (instead of .h file) */
int pcm_c_build_core_event(uint8_t, const char *);
int pcm_c_init();
void pcm_c_start();
void pcm_c_stop();
uint64_t pcm_c_get_cycles(uint32_t);
uint64_t pcm_c_get_instr(uint32_t);
uint64_t pcm_c_get_core_event(uint32_t, uint32_t);
#endif
#endif
