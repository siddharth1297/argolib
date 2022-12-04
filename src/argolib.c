/*
 * Header file for argolib C interfaces.
 */
#define _GNU_SOURCE
#include "argolib.h"

static int get_user_pool_rank(ABT_pool pool);

void argolib_init(int argc, char **argv) {

  // make push() and pop() with pointers,ABT_ppol_pop inside user defined pop is
  // not working
  ABT_init(argc, argv);

  char *streamsStr = getenv("ARGOLIB_WORKERS");
  if (streamsStr == NULL) {
    fprintf(stderr, "ARGOLIB_WORKERS WORKERS NONE\n");
  } else {
    streams = atoi(streamsStr);
  }
  if (streams <= 0)
    streams = 1;

  fprintf(stdout, "ARGOLIB_NUMSTREAMS NUMSTREAMS %d\n", streams);

  mode = 0;
  assert(mode != -1);

  /* Allocate memory. */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * streams);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);
  scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * streams);
  
  if (mode == 0) {
    fprintf(stdout, "ARGOLIB_MODE MODE RAND_WS\n");
    /* Create pools */
    create_pools_1(streams, pools);
    /* Create schedulers */
    create_scheds_1(streams, pools, scheds);
  } else {
    fprintf(stderr, "ARGOLIB_MODE MODE INCORRECT\n");
    assert(0);
  }
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);
pthread_t current_thread = pthread_self();    
   assert(pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) == 0);

  /* Set up a primary execution stream. */
  ABT_xstream_self(&xstreams[0]);

  ABT_xstream_set_main_sched(xstreams[0], scheds[0]); // scheds[0]
	printf("self scheduling done\n");
  /* Create secondary execution streams. */
  for (int i = 1; i < streams; i++) {
    ABT_xstream_create(scheds[i], &xstreams[i]);
  }
  int stat = ABT_xstream_set_cpubind(xstreams[0], 0);
int id = -1;
ABT_xstream_get_cpubind(xstreams[0], &id);
printf("CPUID: %d stat: %d\n", id, stat);
}

void argolib_finalize() {

  for (int i = 1; i < streams; i++) {
    ABT_xstream_join(xstreams[i]);
    ABT_xstream_free(&xstreams[i]);
  }
  for (int i = 1; i < streams; i++) {
        ABT_sched_free((&scheds[i]));
    }

  ABT_finalize();

  /* Free allocated memory. */
  free(xstreams);
  free(pools);
  free(scheds);
}

void argolib_kernel(fork_t fptr, void *args) {

  double t1 = ABT_get_wtime();
  fptr(args);

  finish = 1;
  double t2 = ABT_get_wtime();
  fprintf(stdout, "ARGOLIB_ELAPSEDTIME: %.3f \n", (t2 - t1) * 1.0e3); fflush(stdout);

  size_t total_tasks = 0, total_steals = 0;
  for (int i = 0; i < streams; i++) {
    printf("i %d streams %d\n", i, streams);
    ABT_pool pool = pools[i];
    pool_t *p_pool; 
    ABT_pool_get_data(pool, (void **)&p_pool);
//assert(p_pool != NULL); 
//printf("Got pool\n");
    total_tasks += p_pool->task_count;
    total_steals += p_pool->task_stolen;
/*printf("Updated\n");
    printf("ARGOLIB_INDPOOLCNT pool %d taskCount %ld stealCount %ld\n",
            i, p_pool->task_count, p_pool->task_stolen);
*/
  } 
printf("Done..\n");        
  printf("ARGOLIB_TOTPOOLCNT threads %d taskCount %ld stealCount %ld ratio %0.3lf\n",
          streams, total_tasks, total_steals, ((double)(total_steals) / total_tasks));
}

Task_handle *argolib_fork(fork_t fptr, void *args) {

  int rank;
  ABT_xstream_self_rank(&rank);
  ABT_pool target_pool = pools[rank];
  ABT_thread *child = (ABT_thread *)malloc(sizeof(ABT_thread*));
  assert(child != NULL);
  pool_t *p_pool;
  ABT_pool_get_data(target_pool, (void **)&p_pool);
  (p_pool->task_count)++;

  int x =
      ABT_thread_create(target_pool, fptr, args, ABT_THREAD_ATTR_NULL, child);
  assert(x == ABT_SUCCESS);
  return child;
}

void argolib_join(Task_handle **list, int size) {
  // ABT_thread_join_many() is deprecated.
  // https://www.argobots.org/doxygen/latest/d0/d6d/group__ULT.html#ga7c23f76b44d29ec70a18759ba019b050
  for (int i = 0; i < size; i++) {
    // https://www.argobots.org/doxygen/latest/d0/d6d/group__ULT.html#gaf5275b75a5184bca258e803370e44bea
    int x = ABT_thread_join(*(list[i]));
    assert(x == ABT_SUCCESS);
    // https://www.argobots.org/doxygen/latest/d0/d6d/group__ULT.html#gaf31f748bfd565f97aa8ebabb89e9b632
    assert(ABT_thread_free(list[i]) == ABT_SUCCESS);
    free(list[i]); // Free ABT_thread used in fork
    list[i] = NULL;
  }
}

/******************************************************************************/
/* Normal RAND_WS                                    */
/******************************************************************************/

int sched_init_1(ABT_sched sched, ABT_sched_config config) {

  sched_data_t *p_data = (sched_data_t *)calloc(1, sizeof(sched_data_t));

  ABT_sched_config_read(config, 1, &p_data->event_freq);
  ABT_sched_set_data(sched, (void *)p_data);

  return ABT_SUCCESS;
}

void sched_run_1(ABT_sched sched) {

  int rank;
  ABT_xstream_self_rank(&rank);
  if(rank != 0) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(rank, &cpuset);
    pthread_t current_thread = pthread_self();
    assert(pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) == 0);
 }
  int work_count = 0;
  sched_data_t *p_data;
  ABT_pool *pool;
  int target_pool;
  ABT_bool stop;

  ABT_sched_get_data(sched, (void **)&p_data);
  pool = (ABT_pool *)malloc(1 * sizeof(ABT_pool));
  ABT_sched_get_pools(sched, 1, 0, pool);

  while (1) {

    ABT_thread thread;

    int pop_stat = ABT_pool_pop_thread(pool[0], &thread);
    assert(pop_stat == ABT_SUCCESS);
    if (pop_stat == ABT_SUCCESS && thread != ABT_THREAD_NULL) {
      ABT_self_schedule(thread, pool[0]);

    } else {
      target_pool = rand() % streams;

      if ((ABT_pool_pop_thread_ex(pools[target_pool], &thread,
                                  ABT_POOL_CONTEXT_OWNER_SECONDARY) ==
           ABT_SUCCESS) &&
          (thread != ABT_THREAD_NULL)) {
        ABT_self_schedule(thread, pool[0]);
      }
    }

    if (finish == 1) {
      ABT_sched_finish(sched);
      break;
    }

    if (++work_count >= p_data->event_freq) {
      work_count = 0;
      ABT_xstream_check_events(sched);
    }
  }
  free(pool);
}

int sched_free_1(ABT_sched sched) {
  sched_data_t *p_data;

  ABT_sched_get_data(sched, (void **)&p_data);
  free(p_data);

  return ABT_SUCCESS;
}

void create_scheds_1(int num, ABT_pool *pools, ABT_sched *scheds) {

  ABT_sched_config config;

  int i, k;

  ABT_sched_config_var cv_event_freq = {.idx = 0, .type = ABT_SCHED_CONFIG_INT};
  ABT_sched_def sched_def = {.type = ABT_SCHED_TYPE_ULT,
                             .init = sched_init_1,
                             .run = sched_run_1,
                             .free = sched_free_1,
                             .get_migr_pool = NULL};
  ABT_sched_config_create(&config, cv_event_freq, 10, ABT_sched_config_var_end);

  for (i = 0; i < num; i++)
    ABT_sched_create(&sched_def, 1, &pools[i], config, &scheds[i]);

  ABT_sched_config_free(&config);
}

// Pool functions

ABT_unit pool_create_unit_1(ABT_pool pool, ABT_thread thread) {
  unit_t *p_unit = (unit_t *)calloc(1, sizeof(unit_t));
  if (!p_unit)
    return ABT_UNIT_NULL;
  p_unit->thread = thread;
  return (ABT_unit)p_unit;
}

void pool_free_unit_1(ABT_pool pool, ABT_unit unit) {
  unit_t *p_unit = (unit_t *)unit;
  free(p_unit);
}

ABT_bool pool_is_empty_1(ABT_pool pool) {
  pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  return p_pool->p_head ? ABT_FALSE : ABT_TRUE;
}

ABT_thread pool_pop_1(ABT_pool pool, ABT_pool_context tail) {

  pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_t *p_unit = NULL;
  int own_pool = !(tail == ABT_POOL_CONTEXT_OWNER_SECONDARY);

  if (own_pool) {

    pthread_mutex_lock(&p_pool->lock);
    if (p_pool->p_head == NULL) {
    pthread_mutex_unlock(&p_pool->lock);
      return ABT_THREAD_NULL;
    }

    if (p_pool->p_head == p_pool->p_tail) {
      // Only one thread.
      p_unit = p_pool->p_head;
      p_pool->p_head = NULL;
      p_pool->p_tail = NULL;
    } else {
      // Pop from the head.
      p_unit = p_pool->p_tail;
      p_pool->p_tail = p_unit->p_next;
    }
    pthread_mutex_unlock(&p_pool->lock);

  } else {

    pthread_mutex_lock(&p_pool->lock);

    // Don't steal in case of 0 or 1 task
    if (p_pool->p_head == p_pool->p_tail) {
      pthread_mutex_unlock(&p_pool->lock);
      return ABT_THREAD_NULL;
    }

    // Pop from the head.
    p_unit = p_pool->p_head;
    p_pool->p_head = p_unit->p_prev;

    (p_pool->task_stolen)++;

    pthread_mutex_unlock(&p_pool->lock);
  }

  if (!p_unit)
    return ABT_THREAD_NULL;
  return p_unit->thread;
}

void pool_push_1(ABT_pool pool, ABT_unit unit, ABT_pool_context c) {
  pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_t *p_unit = (unit_t *)unit;

    pthread_mutex_lock(&p_pool->lock);
  if (p_pool->p_tail) {
    p_unit->p_next = p_pool->p_tail;
    p_pool->p_tail->p_prev = p_unit;
  } else {
    p_pool->p_head = p_unit;
  }
  p_pool->p_tail = p_unit;
    pthread_mutex_unlock(&p_pool->lock);
}

int pool_init_1(ABT_pool pool, ABT_pool_config config) {
  pool_t *p_pool = (pool_t *)calloc(1, sizeof(pool_t));
  if (!p_pool)
    return ABT_ERR_MEM;

  int ret = pthread_mutex_init(&p_pool->lock, 0);
  if (ret != 0) {
    free(p_pool);
    return ABT_ERR_SYS;
  }
  ABT_pool_set_data(pool, (void *)p_pool);
  return ABT_SUCCESS;
}

void pool_free_1(ABT_pool pool) {
  pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  pthread_mutex_destroy(&p_pool->lock);
  free(p_pool);
}

void create_pools_1(int num, ABT_pool *pools) {

  ABT_pool_user_def def;
  ABT_pool_user_def_create(pool_create_unit_1, pool_free_unit_1,
                           pool_is_empty_1, pool_pop_1, pool_push_1, &def);
  ABT_pool_user_def_set_init(def, pool_init_1);
  ABT_pool_user_def_set_free(def, pool_free_1);

  ABT_pool_config config;
  ABT_pool_config_create(&config);

  const int automatic = 1;
  ABT_pool_config_set(config, ABT_pool_config_automatic.key,
                      ABT_pool_config_automatic.type, &automatic);

  for (int i = 0; i < num; i++) {

    ABT_pool_create(def, config, &pools[i]);
  }

  ABT_pool_user_def_free(&def);
  ABT_pool_config_free(&config);
}

static int get_user_pool_rank(ABT_pool pool) {
  int pool_idx = 0;
  // TODO: Use pointer arithmetic
  for (; pool_idx < streams; pool_idx++) {
    if (pools[pool_idx] == pool)
      break;
  }
  if (pool_idx == streams) {
    fprintf(stderr, "ARGOLIB_POOLIDNOTFOUND pool %p streams %d\n", pool,
            streams);
    pool_idx = -1;
    for (pool_idx = 0; pool_idx < streams; pool_idx++)
      fprintf(stderr, "ARGOLIB_POOL pool %p idx %d\n", pools[pool_idx],
              pool_idx);
    assert(0);
  }
  return pool_idx;
}
