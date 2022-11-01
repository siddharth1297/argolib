/*
 * Header file for argolib C interfaces.
 */
#include "argolib.h"
#include "argolib_tr_list.h"
#include <unistd.h> // TODO: Remove

static void dump_trace_result(const char *filename, const char *mode,
                              const char *start_msg);
static void set_task_id(ABT_thread thread, counter_t task_id);
static counter_t get_task_id(ABT_thread thread);
static void update_task_id(ABT_thread thread, counter_t task_id);
static void list_aggregation();
static void list_sorting();

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
#ifdef TRACE_REPLAY
  mode = 2;
#endif

  assert(mode != -1);
  /* Allocate memory. */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * streams);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);
  scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * streams);

#ifdef TRACE_REPLAY
  async_counter = (counter_t *)calloc(streams, sizeof(counter_t));
  steal_counter = (counter_t *)calloc(streams, sizeof(counter_t));

  keys = (ABT_key *)calloc(1, sizeof(ABT_key));
  assert(keys != NULL);
  // assert(ABT_key_create(NULL, keys) == ABT_SUCCESS); // Free it explicitly
  assert(ABT_key_create(free, keys) == ABT_SUCCESS); // Free it explicitly
#endif

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
  } else if (mode == 2) {
    printf("Starting with Mode: TRACE_REPLAY\n");
    /* Create pools */
    create_pools_3(streams, pools);
    /* Create schedulers */
    create_scheds_3(streams, pools, scheds);
  } else {
    printf("Incorrect mode\n");
    assert(0);
  }

  printf("Setting self stream\n");
  /* Set up a primary execution stream. */
  ABT_xstream_self(&xstreams[0]);

  printf("Scheduling main stream\n");
  ABT_xstream_set_main_sched(xstreams[0], scheds[0]); // scheds[0]

  printf("Creating new streams\n");
  /* Create secondary execution streams. */
  for (int i = 1; i < streams; i++) {
    ABT_xstream_create(scheds[i], &xstreams[i]);
  }
  printf("===============init Done===============\n");
}

void argolib_finalize() {

  for (int i = 1; i < streams; i++) {
    ABT_xstream_join(xstreams[i]);
    ABT_xstream_free(&xstreams[i]);
  }

  ABT_finalize();

  /* Free allocated memory. */
#ifdef TRACE_REPLAY
  free(async_counter);
  free(steal_counter);
  ABT_key_free(keys);
  free(keys);
#endif
  free(xstreams);
  free(pools);
  free(scheds);
}

void argolib_kernel(fork_t fptr, void *args) {

  double t1 = ABT_get_wtime();
  fptr(args);

  finish = 1;
  printf("Task count :%ld\n", counter);
  double t2 = ABT_get_wtime();
  printf("elapsed time: %.3f \n", (t2 - t1) * 1.0e3);
}

Task_handle *argolib_fork(fork_t fptr, void *args) {

  counter++;

  int rank;
  ABT_xstream_self_rank(&rank);
  ABT_pool target_pool = pools[rank];
  ABT_thread *child = (ABT_thread *)malloc(sizeof(ABT_thread *));

#ifdef TRACE_REPLAY
  if (replay_enabled) {
    assert(0);
  } else {
    assert(ABT_SUCCESS == ABT_thread_create(target_pool, fptr, args,
                                            ABT_THREAD_ATTR_NULL, child));
#ifdef DEBUG
    // TODO: Remove below code after testing
    printf("Child Id: %ld pool: %d\n", get_task_id(*child), rank);

    // printf("Sleep: pool: %d\n", rank);
    // printf("---------------------------\n");
    // sleep(5);
#endif
  }
#else
  assert(ABT_thread_create(target_pool, fptr, args, ABT_THREAD_ATTR_NULL,
                           child) == ABT_SUCCESS);
#endif

  return child;
}

void argolib_join(Task_handle **list, int size) {

  // ABT_thread_join_many() is deprecated.
  // https://www.argobots.org/doxygen/latest/d0/d6d/group__ULT.html#ga7c23f76b44d29ec70a18759ba019b050

  for (int i = 0; i < size; i++) {
    // https://www.argobots.org/doxygen/latest/d0/d6d/group__ULT.html#gaf5275b75a5184bca258e803370e44bea
    int x = ABT_thread_join(*(list[i]));
    if (x != ABT_SUCCESS) {
      printf("RetVal: %d\n", x);
      assert(x == ABT_SUCCESS);
    }
    // TODO: Check it
    /* counter_t *val = NULL;
   assert(ABT_thread_get_specific(*(list[i]), keys[0], (void **)(&val)) ==
          ABT_SUCCESS);
   if(val != NULL)
   free(val);
   printf("Freeing\n");*/
    // https://www.argobots.org/doxygen/latest/d0/d6d/group__ULT.html#gaf31f748bfd565f97aa8ebabb89e9b632
    x = ABT_thread_free(list[i]);
    if (x != ABT_SUCCESS) {
      printf("x = %d\n", x);
      assert(ABT_thread_free(list[i]) == ABT_SUCCESS);
    }
    free(list[i]); // Free ABT_thread used in fork
    list[i] = NULL;
  }
}

void argolib_start_tracing() {
  tracing_enabled = 1;
  for (int i = 0; i < streams; i++) {
    async_counter[i] = i * (COUNTER_MAX / streams);
    printf("async_counter[%d] = %ld\n", i, async_counter[i]);
  }
  memset((void *)steal_counter, 0, streams * sizeof(counter_t));
  printf("===============trace Started===============\n");
}

void argolib_stop_tracing() {
#ifdef DUMP_TRACE
  dump_trace_result("trace_report.txt", "w+", "Steals");
  printf("List original dumped\n");
#endif
  list_aggregation();
  printf("List Aggregation done\n");
#ifdef DUMP_TRACE
  dump_trace_result("trace_report.txt", "a+", "After Aggregation");
  printf("List Aggregation dumped\n");
#endif
  list_sorting();
  printf("List Sorting done\n");
#ifdef DUMP_TRACE
  dump_trace_result("trace_report.txt", "a+", "After Sorting");
  printf("List Sorting dumped\n");
#endif
  printf("===============trace Stopped===============\n");
  return;

  if (replay_enabled == 0) {
    replay_enabled = 1;
  }
}

// Pool functions

ABT_unit pool_create_unit_2(ABT_pool pool, ABT_thread thread) {

  unit_t *p_unit = (unit_t *)calloc(1, sizeof(unit_t));
  if (!p_unit)
    return ABT_UNIT_NULL;
  p_unit->thread = thread;
  return (ABT_unit)p_unit;
}

void pool_free_unit_2(ABT_pool pool, ABT_unit unit) {
  unit_t *p_unit = (unit_t *)unit;
  free(unit);
}

ABT_bool pool_is_empty_2(ABT_pool pool) {
  pool_overhead_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  return p_pool->p_head ? ABT_FALSE : ABT_TRUE;
}

ABT_thread pool_pop_2(ABT_pool pool, ABT_pool_context tail) {

  pool_overhead_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_t *p_unit = NULL;

  if (!(tail & ABT_POOL_CONTEXT_OWNER_SECONDARY)) {

    if (p_pool->p_head == NULL) {

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

  } else {

    if (p_pool->p_head == NULL) {

      return ABT_THREAD_NULL;
    }

    // Pop from the head.

    p_unit = p_pool->p_head;
    p_pool->p_head = p_unit->p_prev;
  }

  //  if(p_pool->id==0)
  //  printf("pop id%d\n",p_pool->id);

  if (!p_unit)
    return ABT_THREAD_NULL;
  p_pool->wu -= 1;
  return p_unit->thread;
}

void pool_push_2(ABT_pool pool, ABT_unit unit, ABT_pool_context c) {
  pool_overhead_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_t *p_unit = (unit_t *)unit;
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

  pool_overhead_t *p_pool =
      (pool_overhead_t *)calloc(1, sizeof(pool_overhead_t));
  p_pool->rb = (request_box_t *)calloc(1, sizeof(request_box_t));

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
  pool_overhead_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  pthread_mutex_destroy(&p_pool->rb->lock);
  pthread_cond_destroy(&p_pool->rb->cond);
  free(p_pool->rb);
  free(p_pool->mailbox);
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

  sched_data_t *p_data = (sched_data_t *)calloc(1, sizeof(sched_data_t));

  ABT_sched_config_read(config, 1, &p_data->event_freq);
  ABT_sched_set_data(sched, (void *)p_data);

  return ABT_SUCCESS;
}

void sched_run_2(ABT_sched sched) {

  int work_count = 0;
  sched_data_t *p_data;
  ABT_pool *pool;
  int target_pool;
  ABT_bool stop;
  pool_overhead_t *p_pool;

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
        pool_overhead_t *thief;
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
      pool_overhead_t *vic_pool;
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

  sched_data_t *p_data;

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

  } else {

    pthread_mutex_lock(&p_pool->lock);

    if (p_pool->p_head == NULL)
      return ABT_THREAD_NULL;

    // Pop from the head.
    p_unit = p_pool->p_head;
    p_pool->p_head = p_unit->p_prev;

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

/******************************************************************************/
/* TRACE_REPLAY                                    */
/******************************************************************************/

// Helper functions

static void dump_trace_result(const char *filename, const char *mode,
                              const char *start_msg) {
  printf("Dumping started. filename: %s msgType: %s\n", filename, start_msg);
  FILE *fp = fopen(filename, mode);
  fprintf(fp, "======================%s=====================\n", start_msg);
  for (int i = 0; i < streams; i++) {
    tracereplay_pool_t *this_pool_data = NULL;
    ABT_pool_get_data(pools[i], (void **)&this_pool_data);
    trace_task_list_t *task_list = this_pool_data->task_list, *p = task_list;
    fprintf(fp, "\n-----Stream: %d -> %p %p-----\n", i,
            this_pool_data->task_list, task_list);
    printf("at stream %d \n", i);
    // GOTO last
    /*while (p && p->next) {
      p = p->next;
    }*/
    while (p) {
      trace_data_t trace_data = p->trace_data;
      fprintf(fp, "{%ld\t\t%d\t%d\t\t%ld} \n", trace_data.task_id,
              trace_data.create_pool_idx, trace_data.exec_pool_idx,
              trace_data.steal_count);
      // p = p->prev;
      p = p->next;
    }
    fprintf(fp, "\n\n");
  }
  fclose(fp);
  printf("Dumping done. filename: %s msgType: %s\n", filename, start_msg);
}

static void list_aggregation() {
  printf("LIST aggregation started\n");
  for (int i = 0; i < streams; i++) {
    tracereplay_pool_t *this_pool_data = NULL;
    ABT_pool_get_data(pools[i], (void **)&this_pool_data);
    trace_task_list_t *curr = this_pool_data->task_list, *nxt = NULL;
    while (curr) {
      nxt = curr->next;
      if (curr->trace_data.create_pool_idx != curr->trace_data.exec_pool_idx) {
        remove_node_from_trace_list(&(this_pool_data->task_list), curr);
        tracereplay_pool_t *source_pool_data = NULL;
        ABT_pool_get_data(pools[curr->trace_data.create_pool_idx],
                          (void **)&source_pool_data);
        add_to_trace_list(&(source_pool_data->task_list), curr);
      }
      curr = nxt;
    }
  }
  //#ifdef DEBUG TODO: Uncomment in live
  // Verify aggregation
  for (int i = 0; i < streams; i++) {
    tracereplay_pool_t *this_pool_data = NULL;
    ABT_pool_get_data(pools[i], (void **)&this_pool_data);
    trace_task_list_t *curr = this_pool_data->task_list;
    while (curr) {
      assert(curr->trace_data.create_pool_idx == i);
      curr = curr->next;
    }
  }
  printf("List Aggregation Verified\n");
  //#endif
}

static void list_sorting() {
  printf("LIST sort started\n");
  for (int i = 0; i < streams; i++) {
    tracereplay_pool_t *this_pool_data = NULL;
    ABT_pool_get_data(pools[i], (void **)&this_pool_data);
    insertionSort(&(this_pool_data->task_list));
  }
  //#ifdef DEBUG TODO: Uncomment in live
  // Verify sort
  for (int i = 0; i < streams; i++) {
    tracereplay_pool_t *this_pool_data = NULL;
    ABT_pool_get_data(pools[i], (void **)&this_pool_data);
    trace_task_list_t *curr = this_pool_data->task_list;
    counter_t lastVal = COUNTER_MIN;
    while (curr) {
      assert(lastVal < curr->trace_data.task_id);
      lastVal = curr->trace_data.task_id;
      curr = curr->next;
    }
  }
  printf("List Sorting Verified\n");
  //#endif
}

static void set_task_id(ABT_thread thread, counter_t task_id) {
  /*	void *data;
          int ret;
          assert(ABT_thread_get_specific(thread, keys[0], &data) ==
     ABT_SUCCESS); assert(data == NULL); if (data) free(data);*/
  counter_t *val = (counter_t *)malloc(sizeof(counter_t));
  memcpy(val, &task_id, sizeof(task_id));
  ABT_key key = keys[0];
  // assert(ABT_SUCCESS == ABT_key_set(key, val));
  int set_stat = ABT_thread_set_specific(thread, key, val);
  assert(ABT_SUCCESS == set_stat);
}

static void update_task_id(ABT_thread thread, counter_t task_id) {
  counter_t *val = NULL;
  assert(ABT_thread_get_specific(thread, keys[0], (void **)(&val)) ==
         ABT_SUCCESS);
  free(val);

  set_task_id(thread, task_id);
#ifdef DEBUG
  assert(get_task_id(thread) == task_id);
#endif
}

static counter_t get_task_id(ABT_thread thread) {
  counter_t *val = NULL;
  assert(ABT_thread_get_specific(thread, keys[0], (void **)(&val)) ==
         ABT_SUCCESS);
  return val ? *val : -1;
}

static int get_user_pool_rank(ABT_pool pool) {
  int pool_idx = 0;
  // TODO: Use pointer arithmetic
  for (; pool_idx < streams; pool_idx++) {
    if (pools[pool_idx] == pool)
      break;
  }
  if (pool_idx == streams) {
    printf("PoolIdNotFound pool: %p\n", pool);
    pool_idx = -1;
    assert(0);
  }
  return pool_idx;
}

// Pool functions

ABT_unit pool_create_unit_3(ABT_pool pool, ABT_thread thread) {
  tracereplay_unit_t *p_unit =
      (tracereplay_unit_t *)calloc(1, sizeof(tracereplay_unit_t));
  if (!p_unit)
    return ABT_UNIT_NULL;
  p_unit->thread = thread;

  int rank = get_user_pool_rank(pool);
  counter_t task_id = ++(async_counter[rank]);
  set_task_id(thread, task_id);
#ifdef DEBUG
  if (tracing_enabled) {
    int rank = get_user_pool_rank(pool);
    printf("Created Id: %ld pool: %d\n", get_task_id(p_unit->thread), rank);
  }
#endif
  return (ABT_unit)p_unit;
}

void pool_free_unit_3(ABT_pool pool, ABT_unit unit) {
  tracereplay_unit_t *p_unit = (tracereplay_unit_t *)unit;
  /*
        counter_t *val = NULL;
  assert(ABT_thread_get_specific(p_unit->thread, keys[0], (void **)(&val)) ==
         ABT_SUCCESS);
  if(val != NULL)
  free(val);
*/
  free(p_unit);
}

ABT_bool pool_is_empty_3(ABT_pool pool) {
  tracereplay_pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  return p_pool->p_head ? ABT_FALSE : ABT_TRUE;
}

ABT_thread pool_pop_3(ABT_pool pool, ABT_pool_context tail) {
  // printf("%d: At poolpop \n", __LINE__);

  tracereplay_pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  tracereplay_unit_t *p_unit = NULL;

  // TODO: Fix this part
  if (!(tail & ABT_POOL_CONTEXT_OWNER_SECONDARY)) {
    // thread is accessing its own pool
    if (p_pool->p_head == NULL) {
      return ABT_THREAD_NULL;
    }

    pthread_mutex_lock(&p_pool->lock);
    if (p_pool->p_head == p_pool->p_tail) {
      // Only one thread.
      p_unit = p_pool->p_head;
      p_pool->p_head = NULL;
      p_pool->p_tail = NULL;
    } else {
      // Pop from the tail.
      p_unit = p_pool->p_tail;
      p_pool->p_tail = p_unit->p_next;
    }
    pthread_mutex_unlock(&p_pool->lock);
  } else {
    // TODO: Add single thread condition
    pthread_mutex_lock(&p_pool->lock);
    if (p_pool->p_head == NULL) {
      pthread_mutex_unlock(&p_pool->lock);
      return ABT_THREAD_NULL;
    }
    // Pop from the head.
    p_unit = p_pool->p_head;
    p_pool->p_head = p_unit->p_prev;

    pthread_mutex_unlock(&p_pool->lock);
  }

  if (!p_unit)
    return ABT_THREAD_NULL;
#ifdef DEBUG
  printf("Popped Id: %ld pool: %d\n", get_task_id(p_unit->thread),
         get_user_pool_rank(pool));
#endif
  return p_unit->thread;
}

void pool_push_3(ABT_pool pool, ABT_unit unit, ABT_pool_context c) {
  // printf("%d: At poolpush \n", __LINE__);
  tracereplay_pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  tracereplay_unit_t *p_unit = (tracereplay_unit_t *)unit;

  // TODO: Fix it
  pthread_mutex_lock(&p_pool->lock);
  if (p_pool->p_tail) {
#ifdef DEBUG
    printf("Pushed(none) Id: %ld pool: %d\n", get_task_id(p_unit->thread),
           get_user_pool_rank(pool));
#endif
    p_unit->p_next = p_pool->p_tail;
    p_pool->p_tail->p_prev = p_unit;
  } else {
#ifdef DEBUG
    printf("Pushed(empt) Id: %ld pool: %d\n", get_task_id(p_unit->thread),
           get_user_pool_rank(pool));
#endif
    p_pool->p_head = p_unit;
  }
  p_pool->p_tail = p_unit;
  pthread_mutex_unlock(&p_pool->lock);
}

int pool_init_3(ABT_pool pool, ABT_pool_config config) {
  printf("%d: At poolinit %p\n", __LINE__, pool);

  tracereplay_pool_t *p_pool =
      (tracereplay_pool_t *)calloc(1, sizeof(tracereplay_pool_t));
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

void pool_free_3(ABT_pool pool) {
  tracereplay_pool_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  pthread_mutex_destroy(&p_pool->lock);
  freeList(p_pool->task_list);
  free(p_pool);
}

void create_pools_3(int num, ABT_pool *pools) {
  printf("%d: At createpool %p\n", __LINE__, pools);

  ABT_pool_user_def def;
  ABT_pool_user_def_create(pool_create_unit_3, pool_free_unit_3,
                           pool_is_empty_3, pool_pop_3, pool_push_3, &def);
  ABT_pool_user_def_set_init(def, pool_init_3);
  ABT_pool_user_def_set_free(def, pool_free_3);

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

int sched_init_3(ABT_sched sched, ABT_sched_config config) {
  printf("%d: At sched_init %p\n", __LINE__, sched);

  sched_data_t *p_data = (sched_data_t *)calloc(1, sizeof(sched_data_t));

  ABT_sched_config_read(config, 1, &p_data->event_freq);
  ABT_sched_set_data(sched, (void *)p_data);

  return ABT_SUCCESS;
}

void sched_run_3(ABT_sched sched) {
  printf("%d: At schedule %p\n", __LINE__, sched);

  sched_data_t *p_data;
  ABT_sched_get_data(sched, (void **)&p_data);

  ABT_pool *pool = (ABT_pool *)malloc(1 * sizeof(ABT_pool));
  ABT_sched_get_pools(sched, 1, 0, pool);

  tracereplay_pool_t *this_pool_data = NULL;
  ABT_pool_get_data(pool[0], (void **)&this_pool_data);

  int work_count = 0, this_pool_idx = get_user_pool_rank(pool[0]),
      vic_pool_idx = 0;

  while (1) {
    if (replay_enabled) {
      assert(0);
    } else {
      ABT_thread thread = NULL;
      ABT_unit unit;

      // Pop from own pool
      assert(ABT_pool_pop(pool[0], &unit) == ABT_SUCCESS);
      int thread_stat = ABT_unit_get_thread(unit, &thread);
      if (thread_stat == ABT_SUCCESS && thread != ABT_THREAD_NULL) {
        counter_t task_id = get_task_id(thread);
#ifdef DEBUG
        printf("Got(Own) pool: %d, taskId: %ld\n", this_pool_idx, task_id);
#endif
        ABT_self_schedule(thread, pool[0]);
      } else {
        // Steal from victim pool TODO: what if it choose itself ?
        vic_pool_idx = 0; // TODO: Set it rand() / streams;
        if (vic_pool_idx != this_pool_idx) {
          int pool_pop_res = ABT_pool_pop(pools[vic_pool_idx], &unit);

          if (pool_pop_res == ABT_SUCCESS) {
            int thread_stat = ABT_unit_get_thread(unit, &thread);
            if (thread_stat == ABT_SUCCESS && thread != ABT_THREAD_NULL) {
              counter_t task_id = get_task_id(thread);
#ifdef DEBUG
              printf("Got(Stole) pool(vic): %d pool(own): %d taskId: %ld\n",
                     vic_pool_idx, this_pool_idx, task_id);
#endif
              counter_t new_task_id = ++(async_counter[this_pool_idx]);
              update_task_id(thread, new_task_id);

              // DO the things
              counter_t steal_count = (steal_counter[this_pool_idx])++;
              add_to_trace_list(&(this_pool_data->task_list),
                                create_trace_data_node(task_id, vic_pool_idx,
                                                       this_pool_idx,
                                                       steal_count));
              // ABT_pool_push(pool[0], unit); // Push in its own pool
              ABT_self_schedule(thread, pool[0]);
            }
          }
        }
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

int sched_free_3(ABT_sched sched) {

  sched_data_t *p_data;

  ABT_sched_get_data(sched, (void **)&p_data);
  free(p_data);

  return ABT_SUCCESS;
}

void create_scheds_3(int num, ABT_pool *pools, ABT_sched *scheds) {

  ABT_sched_config config;

  int i, k;

  ABT_sched_config_var cv_event_freq = {.idx = 0, .type = ABT_SCHED_CONFIG_INT};
  ABT_sched_def sched_def = {.type = ABT_SCHED_TYPE_ULT,
                             .init = sched_init_3,
                             .run = sched_run_3,
                             .free = sched_free_3,
                             .get_migr_pool = NULL};
  ABT_sched_config_create(&config, cv_event_freq, 10, ABT_sched_config_var_end);

  for (i = 0; i < num; i++)
    ABT_sched_create(&sched_def, 1, &pools[i], config, &scheds[i]);

  ABT_sched_config_free(&config);
}
