/*
 * Header file for argolib C interfaces.
 */
#ifndef ARGOLIB_H
#define ARGOLIB_H

#include "argolibdefs.h"

static int streams = 0;
static ABT_xstream *xstreams = NULL;
static ABT_pool *pools = NULL;
static ABT_sched *scheds = NULL;
static size_t counter = 0;
static int finish = 0;
static int pool_id = 0;
static int mode =
    -1; // 0 for normal RAND_WS, 1 for private deque RAND_WS, 2 for TRACE_REPLAY
static int tracing_enabled = 0;
static int replay_enabled = 0;
static counter_t *async_counter = NULL;
static counter_t *steal_counter = NULL;
static ABT_key *keys = NULL;

/**
 * Initializes the ArgoLib runtime, and it should be the first thing to call in
 * the user main. Arguments “argc” and “argv” are the ones passed in the call to
 * user main method.
 */
void argolib_init(int argc, char **argv);

/**
 * Finalize the ArgoLib runtime, and performs the cleanup.
 */
void argolib_finalize();

/**
 * User can use this API to launch the top-level computation kernel. This API
 * gathers statistics for the parallel execution and reports after the
 * completion of the computation kernel. Some of the statistics are: execution
 * time, total tasks created, etc. This top-level kernel would actually be
 * launching the recursive tasks.
 */
void argolib_kernel(fork_t fptr, void *args);

/**
 * Creates an Argobot ULT that would execute a user method with the specified
 * argument. It returns a pointer to the task handle that would be used for
 * joining this ULT.
 */
Task_handle *argolib_fork(fork_t fptr, void *args);

/**
 * Used for joining one more ULTs using the corresponding task handles. In case
 * of more than one task handles, user can pass an array of Task_handle*. The
 * parameter “size” is the array size.
 */
void argolib_join(Task_handle **list, int size);

/**
 * Used for start tracing
 */
void argolib_start_tracing();

/**
 * Used for stop tracing
 */
void argolib_stop_tracing();

/* Private functions. Not to be used user */

// Pool functions
ABT_unit pool_create_unit_2(ABT_pool pool, ABT_thread thread);
void pool_free_unit_2(ABT_pool pool, ABT_unit unit);
ABT_bool pool_is_empty_2(ABT_pool pool);
ABT_thread pool_pop_2(ABT_pool pool, ABT_pool_context tail);
void pool_push_2(ABT_pool pool, ABT_unit unit, ABT_pool_context c);
int pool_init_2(ABT_pool pool, ABT_pool_config config);
void pool_free_2(ABT_pool pool);
void create_pools_2(int num, ABT_pool *pools);

// scheduler
int sched_init_2(ABT_sched sched, ABT_sched_config config);
void sched_run_2(ABT_sched sched);
int sched_free_2(ABT_sched sched);
void create_scheds_2(int num, ABT_pool *pools, ABT_sched *scheds);

/******************************************************************************/
/* Normal RAND_WS                                    */
/******************************************************************************/

// scheduler
int sched_init_1(ABT_sched sched, ABT_sched_config config);
void sched_run_1(ABT_sched sched);
int sched_free_1(ABT_sched sched);
void create_scheds_1(int num, ABT_pool *pools, ABT_sched *scheds);

// Pool functions
ABT_unit pool_create_unit_1(ABT_pool pool, ABT_thread thread);
void pool_free_unit_1(ABT_pool pool, ABT_unit unit);
ABT_bool pool_is_empty_1(ABT_pool pool);
void pool_push_1(ABT_pool pool, ABT_unit unit, ABT_pool_context c);
int pool_init_1(ABT_pool pool, ABT_pool_config config);
void pool_free_1(ABT_pool pool);
void create_pools_1(int num, ABT_pool *pools);

/******************************************************************************/
/* TRACE_RPLAY                                    */
/******************************************************************************/

// scheduler
int sched_init_3(ABT_sched sched, ABT_sched_config config);
void sched_run_3(ABT_sched sched);
int sched_free_3(ABT_sched sched);
void create_scheds_3(int num, ABT_pool *pools, ABT_sched *scheds);

// Pool functions
ABT_unit pool_create_unit_3(ABT_pool pool, ABT_thread thread);
void pool_free_unit_3(ABT_pool pool, ABT_unit unit);
ABT_bool pool_is_empty_3(ABT_pool pool);
void pool_push_3(ABT_pool pool, ABT_unit unit, ABT_pool_context c);
int pool_init_3(ABT_pool pool, ABT_pool_config config);
void pool_free_3(ABT_pool pool);
void create_pools_3(int num, ABT_pool *pools);

#endif
