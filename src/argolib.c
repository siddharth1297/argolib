/*
 * Header file for argolib C interfaces.
 */
#include "argolib.h"

void argolib_init(int argc, char **argv) {

  // make push() and pop() with pointers,ABT_ppol_pop inside user defined pop is
  // not working
  ABT_init(argc, argv);

  char *streamsStr = getenv("ARGOLIB_WORKERS");
  if (streamsStr == NULL) {
    fprintf(stderr, "ARGOLIB_WORKERS is not set\n");
  } else {
    streams = atoi(streamsStr);
  }
  if (streams <= 0)
    streams = 1;

  printf("No of streams = %d\n", streams);

  mode = 0;
#ifdef MODE_PRIVATE_DQ
  mode = 1;
#endif

  assert(mode != -1);
  /* Allocate memory. */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * streams);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);
  scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * streams);

  if (mode == 0) {
    printf("Starting with Mode: RAND_WS\n");
    /* Create pools */
    create_pools_1(streams, pools);
    /* Create schedulers */
    create_scheds_1(streams, pools, scheds);
  } else if (mode == 1) {
    printf("Starting with Mode: PRIVATE_DQ_RAND_WS\n");
    /* Create pools */
    create_pools_2(streams, pools);
    /* Create schedulers */
    create_scheds_2(streams, pools, scheds);
  } else {
    printf("Incorrect mode\n");
    assert(0);
  }

  /* Set up a primary execution stream. */
  ABT_xstream_self(&xstreams[0]);

  ABT_xstream_set_main_sched(xstreams[0], scheds[0]); // scheds[0]

  /* Create secondary execution streams. */
  for (int i = 1; i < streams; i++) {
    ABT_xstream_create(scheds[i], &xstreams[i]);
  }
}

void argolib_finalize() {

  for (int i = 1; i < streams; i++) {
    ABT_xstream_join(xstreams[i]);
    ABT_xstream_free(&xstreams[i]);
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
  printf("Task count :%d\n", counter);
  double t2 = ABT_get_wtime();
  printf("elapsed time: %.3f \n", (t2 - t1) * 1.0e3);
}

Task_handle *argolib_fork(fork_t fptr, void *args) {

  counter++;

  int rank;
  ABT_xstream_self_rank(&rank);
  ABT_pool target_pool = pools[rank];
  ABT_thread *child = (ABT_thread *)malloc(sizeof(ABT_thread *));

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

// Pool functions

ABT_unit pool_create_unit_2(ABT_pool pool, ABT_thread thread) {

  unit_tt *p_unit = (unit_tt *)calloc(1, sizeof(unit_tt));
  if (!p_unit)
    return ABT_UNIT_NULL;
  p_unit->thread = thread;
  return (ABT_unit)p_unit;
}

void pool_free_unit_2(ABT_pool pool, ABT_unit unit) {
  unit_tt *p_unit = (unit_tt *)unit;
  free(unit);
}

ABT_bool pool_is_empty_2(ABT_pool pool) {
  pool_overhead *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  return p_pool->p_head ? ABT_FALSE : ABT_TRUE;
}

ABT_thread pool_pop_2(ABT_pool pool, ABT_pool_context tail) {

  pool_overhead *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_tt *p_unit = NULL;

  if (!(tail & ABT_POOL_CONTEXT_OWNER_SECONDARY)) {

    if (p_pool->p_head == NULL) {

      return ABT_THREAD_NULL;

    }

    else if (p_pool->p_head == p_pool->p_tail) {
      // Only one thread.

      p_unit = p_pool->p_head;
      p_pool->p_head = NULL;
      p_pool->p_tail = NULL;
    }

    else {
      // Pop from the head.

      p_unit = p_pool->p_tail;
      p_pool->p_tail = p_unit->p_next;
    }

  } else {

    if (p_pool->p_head == NULL) {

      return ABT_THREAD_NULL;
    }

    else {
      // Pop from the head.

      p_unit = p_pool->p_head;
      p_pool->p_head = p_unit->p_prev;
    }
  }

  //  if(p_pool->id==0)
  //  printf("pop id%d\n",p_pool->id);

  if (!p_unit)
    return ABT_THREAD_NULL;
  p_pool->wu -= 1;
  return p_unit->thread;
}

void pool_push_2(ABT_pool pool, ABT_unit unit, ABT_pool_context c) {
  pool_overhead *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_tt *p_unit = (unit_tt *)unit;
  // printf("pool id%d\n",p_pool->id);

  if (p_pool->p_tail) {
    p_unit->p_next = p_pool->p_tail;
    p_pool->p_tail->p_prev = p_unit;
  } else {
    p_pool->p_head = p_unit;
  }
  p_pool->p_tail = p_unit;
  p_pool->wu += 1;

  // printf("push2\n");
}

int pool_init_2(ABT_pool pool, ABT_pool_config config) {

  pool_overhead *p_pool = (pool_overhead *)calloc(1, sizeof(pool_overhead));
  p_pool->rb = (request_box *)calloc(1, sizeof(request_box));

  if (!p_pool)
    return ABT_ERR_MEM;

  int ret = pthread_mutex_init(&(p_pool->rb->lock), 0);

  p_pool->wu = 0;
  p_pool->counter = 0;
  int ret2 = pthread_cond_init(&p_pool->rb->cond, 0);
  p_pool->mailbox = (Task_handle *)malloc(sizeof(Task_handle) * 5);
  p_pool->id = pool_id++;
  p_pool->rb->thief_id = -1;

  if (ret != 0 || ret2 != 0) {
    free(p_pool);
    return ABT_ERR_SYS;
  }
  ABT_pool_set_data(pool, (void *)p_pool);

  return ABT_SUCCESS;
}

void pool_free_2(ABT_pool pool) {
  pool_overhead *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  pthread_mutex_destroy(&p_pool->rb->lock);
  pthread_cond_destroy(&p_pool->rb->cond);
  free(p_pool);
}

void create_pools_2(int num, ABT_pool *pools) {

  ABT_pool_user_def def;
  ABT_pool_user_def_create(pool_create_unit_2, pool_free_unit_2,
                           pool_is_empty_2, pool_pop_2, pool_push_2, &def);
  ABT_pool_user_def_set_init(def, pool_init_2);
  ABT_pool_user_def_set_free(def, pool_free_2);

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

// Scheduler

int sched_init_2(ABT_sched sched, ABT_sched_config config) {

  sched_data_tt *p_data = (sched_data_tt *)calloc(1, sizeof(sched_data_tt));

  ABT_sched_config_read(config, 1, &p_data->event_freq);
  ABT_sched_set_data(sched, (void *)p_data);

  return ABT_SUCCESS;
}

void sched_run_2(ABT_sched sched) {

  int work_count = 0;
  sched_data_tt *p_data;
  ABT_pool *pool;
  int target_pool;
  ABT_bool stop;
  pool_overhead *p_pool;

  ABT_sched_get_data(sched, (void **)&p_data);
  pool = (ABT_pool *)malloc(1 * sizeof(ABT_pool));
  ABT_sched_get_pools(sched, 1, 0, pool);
  ABT_pool_get_data(pool[0], (void **)&p_pool);

  while (1) {
    // printf("reached\n");
    ABT_thread thread;
    ABT_unit unit;

    ABT_pool_pop(pool[0], &unit);
    ABT_unit_get_thread(unit, &thread);

    if (thread != ABT_THREAD_NULL) {
      // printf("while if\n");
      pthread_mutex_lock(&p_pool->rb->lock);
      if (p_pool->rb->thief_id != -1) {
        //   printf("%d\n",p_pool->rb->thief_id);
        ABT_pool thief_pool;
        pool_overhead *thief;
        thief_pool = pools[p_pool->rb->thief_id];
        ABT_pool_get_data(thief_pool, (void **)&thief);
        int idx = 0;

        for (int i = 0; i < 5; i++) {

          ABT_thread t;
          ABT_unit u;
          ABT_pool_pop(pool[0], &u);
          ABT_unit_get_thread(u, &t);
          if (t == ABT_THREAD_NULL)
            break;

          else
            thief->mailbox[idx++] = t;
        }

        thief->counter = idx;
        p_pool->rb->thief_id = -1;
        pthread_cond_signal(&p_pool->rb->cond);
      }
      pthread_mutex_unlock(&p_pool->rb->lock);

      // printf("Self schedule\n");
      ABT_self_schedule(thread, pool[0]);

    } else {
      printf("while else\n");
      target_pool = rand() % streams;
      pool_overhead *vic_pool;
      ABT_pool_get_data(pools[target_pool], (void **)&vic_pool);

      if (vic_pool->rb->thief_id != -1 || vic_pool->wu <= 1)
        continue;

      pthread_mutex_lock(&vic_pool->rb->lock);
      vic_pool->rb->thief_id = p_pool->id;

      pthread_cond_wait(&vic_pool->rb->cond, &vic_pool->rb->lock);

      if (p_pool->counter >= 1) {
        ABT_thread t;
        for (int i = 0; i < p_pool->counter; i++) {
          t = p_pool->mailbox[i];
          ABT_unit u;
          ABT_thread_get_unit(t, &u);
          ABT_pool_push(pool[0], u);
        }
      }

      p_pool->counter = 0;

      // printf("stolen task\n");
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

int sched_free_2(ABT_sched sched) {

  sched_data_tt *p_data;

  ABT_sched_get_data(sched, (void **)&p_data);
  free(p_data);

  return ABT_SUCCESS;
}

void create_scheds_2(int num, ABT_pool *pools, ABT_sched *scheds) {

  ABT_sched_config config;

  int i, k;

  ABT_sched_config_var cv_event_freq = {.idx = 0, .type = ABT_SCHED_CONFIG_INT};
  ABT_sched_def sched_def = {.type = ABT_SCHED_TYPE_ULT,
                             .init = sched_init_2,
                             .run = sched_run_2,
                             .free = sched_free_2,
                             .get_migr_pool = NULL};
  ABT_sched_config_create(&config, cv_event_freq, 10, ABT_sched_config_var_end);

  for (i = 0; i < num; i++)
    ABT_sched_create(&sched_def, 1, &pools[i], config, &scheds[i]);

  ABT_sched_config_free(&config);
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
    ABT_unit unit;
    ABT_pool_pop(pool[0], &unit);
    ABT_unit_get_thread(unit, &thread);

    if (thread != ABT_THREAD_NULL) {

      ABT_self_schedule(thread, pool[0]);

    } else {
      target_pool = rand() % streams;

      ABT_pool_pop(pools[target_pool], &unit);
      ABT_unit_get_thread(unit, &thread);

      if (thread != ABT_THREAD_NULL) {

        ABT_pool_push(pool[0], unit);
      }

      // printf("stolen task\n");
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

  if (!(tail & ABT_POOL_CONTEXT_OWNER_SECONDARY)) {

    if (p_pool->p_head == NULL)
      return ABT_THREAD_NULL;

    else if (p_pool->p_head == p_pool->p_tail) {
      // Only one thread.
      p_unit = p_pool->p_head;
      p_pool->p_head = NULL;
      p_pool->p_tail = NULL;
    } else {
      // Pop from the head.
      p_unit = p_pool->p_tail;
      p_pool->p_tail = p_unit->p_next;
    }

  }

  else {

    pthread_mutex_lock(&p_pool->lock);

    if (p_pool->p_head == NULL)
      return ABT_THREAD_NULL;

    else {
      // Pop from the head.
      p_unit = p_pool->p_head;
      p_pool->p_head = p_unit->p_prev;
    }

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

  if (p_pool->p_tail) {
    p_unit->p_next = p_pool->p_tail;
    p_pool->p_tail->p_prev = p_unit;
  } else {
    p_pool->p_head = p_unit;
  }
  p_pool->p_tail = p_unit;
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
