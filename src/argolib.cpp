/*
 * Header file for argolib C interfaces.
 */
#include "argolib.h"

void awake_argolib_workers(int w){

  for(int i=1;i<streams &&w>0;i++){

    pool_energy_t *p_pool;

     ABT_pool_get_data(pools[i], (void **)&p_pool);

     if(p_pool->active==0) {
#ifdef DEBUG
      fprintf("Xstream number %d is going to be awaken\n",i);
#endif
      pthread_mutex_lock(&p_pool->active_lock);
      p_pool->active=1;

      
      
      pthread_mutex_unlock(&p_pool->active_lock);
      pthread_cond_signal(&p_pool->cond);


      w--;
     }

     //free(p_pool);


  }

}

void sleep_argolib_workers(int w){

 

  for(int i=1;i<streams &&w>0;i++){

    pool_energy_t *p_pool;

     ABT_pool_get_data(pools[i], (void **)&p_pool);

     if(p_pool->active==1) {
#ifdef DEBUG
      fprintf("Xstream number %d is going to sleep\n",i);
#endif
      p_pool->active=0;
      w--;
     }

     //free(p_pool);


  }
}

void* daemon_profiler(void *arg){

 // const int fixed_interval= 20ms; //20 ms

  std::this_thread::sleep_for(std::chrono::seconds(1)); //warmup

  double JPI_prev=0;

  while(finish==0){

   // continue;

    
     ___after_sstate = pcm::getSystemCounterState();
    double JPI_curr=getConsumedJoules(___before_sstate, ___after_sstate);
    
    configure_DOP(JPI_prev,JPI_curr);
    JPI_prev=JPI_curr;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ___before_sstate = pcm::getSystemCounterState();
  }

  awake_argolib_workers(streams);
  return NULL;
}

void configure_DOP( double JPI_prev,double JPI_curr){

  
   int wchange=0;

  if(first_configureDOP==1){

    first_configureDOP=0;
   
   wchange=wactive/2;
    sleep_argolib_workers(wchange);
    wactive=wchange;
    lastAction=0;
    return;
  }

  if(JPI_curr<JPI_prev){

    if(lastAction==0){
      
      wchange=wactive/2;
      if(wactive<=wchange) return;
      wactive-=wchange;
      
      sleep_argolib_workers(wchange);
    }

    else{ 
      
      wchange=(streams+wactive)>>1;
      if(wactive-wchange>streams) return;
      wactive+=wchange;
      awake_argolib_workers(wchange);  
    }  
  }

  else{

    if(lastAction==0){
      wchange=(streams+wactive)>>1;
      if(wactive-wchange>streams) return;
      wactive+=wchange;
      awake_argolib_workers(wchange);  
      lastAction=1;
    }

    else{

      wchange=wactive/2;
      if(wactive<=wchange) return;
      wactive-=wchange;
      sleep_argolib_workers(wchange);
      lastAction=0;

    }
  }


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

  fprintf(stdout, "ARGOLIB_NUMSTREAMS NUMSTREAMS %d\n",streams);

   wactive=streams;


  /* Allocate memory. */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * streams);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);
  scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * streams);
  active=(int *)malloc(sizeof(int)*streams);

  for(int i=0;i<streams;i++) active[i]=1;


  fprintf(stdout, "ARGOLIB_MODE MODE RAND_WS\n");

  /* Create pools */
  create_pools_1(streams, pools);
  /* Create schedulers */
  create_scheds_1(streams, pools, scheds);

  /* Set up a primary execution stream. */
  ABT_xstream_self(&xstreams[0]);

  ABT_xstream_set_main_sched(xstreams[0], scheds[0]); // scheds[0]

  /* Create secondary execution streams. */
  for (int i = 1; i < streams; i++) {
    ABT_xstream_create(scheds[i], &xstreams[i]);
  }

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(0, &cpuset);

  //TODO: Make _GNU_SOURCE work for Cpp
  pthread_create(&profiler_thread,NULL,&daemon_profiler,NULL);
  pthread_setaffinity_np(profiler_thread, sizeof(cpu_set_t), &cpuset);

    ___pcm = pcm::PCM::getInstance();
        ___pcm->resetPMU();
    // program() creates common semaphore for the singleton, so ideally to be called before any other references to PCM
    pcm::PCM::ErrorCode status = ___pcm->program();

    switch (status)
    {
    case pcm::PCM::Success:
        //std::cerr << "Success initializing PCM" << std::endl;
        break;
    case pcm::PCM::MSRAccessDenied:
        fprintf(stderr, "Access to Processor Counter Monitor has denied (no MSR or PCI CFG space access)./n");
        exit(EXIT_FAILURE);
    case pcm::PCM::PMUBusy:
        fprintf(stderr, "Access to Processor Counter Monitor has denied (Performance Monitoring Unit is occupied by other application). Try to stop the application that uses PMU.\n");
        fprintf(stderr, "Alternatively you can try running PCM with option -r to reset PMU configuration at your own risk.\n");
        exit(EXIT_FAILURE);
    default:
        fprintf(stderr, "Access to Processor Counter Monitor has denied (Unknown error).\n");
        exit(EXIT_FAILURE);
    }

  

}

void argolib_finalize() {

  

 
  
  for (int i = 1; i < streams; i++) {
    
    int x=ABT_xstream_join(xstreams[i]);
    assert(x==ABT_SUCCESS);
    ABT_xstream_free(&xstreams[i]);
  }

 
  pthread_join(profiler_thread,NULL);

  

  for(int i=0;i<streams;i++){

    pool_energy_t *p_pool;

    ABT_pool_get_data(pools[i], (void **)&p_pool);

    fprintf(stdout, "ARGOLIB_INDPOOLCNT pool %d taskCount %d stealCount %d\n",
            get_user_pool_rank(pools[i]), p_pool->task_count, p_pool->task_stolen);

    counter+=p_pool->task_count;
    steal_counter+=p_pool->task_stolen;
  }

  fprintf(stdout, "ARGOLIB_TOTPOOLCNT taskCount %d stealCount %d\n", counter, steal_counter);
  

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
 
  //printf("Task count :%d\n", counter);
  double t2 = ABT_get_wtime();
 // printf("elapsed time: %.3f \n", (t2 - t1) * 1.0e3);
  fprintf(stdout, "ARGOLIB_ELAPSEDTIME: %.3f\n",(t2 - t1) * 1.0e3);
}

Task_handle *argolib_fork(fork_t fptr, void *args) {

  pool_energy_t *p_pool;

  int rank;
  ABT_xstream_self_rank(&rank);
  ABT_pool target_pool = pools[rank];
  ABT_pool_get_data(target_pool, (void **)&p_pool);
  ABT_thread *child = (ABT_thread *)malloc(sizeof(ABT_thread *));
  p_pool->task_count+=1;

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

  int work_count = 0;
  sched_data_t *p_data;
  ABT_pool *pool;
  int target_pool;
  ABT_bool stop;
  pool_energy_t *p_pool;

  ABT_sched_get_data(sched, (void **)&p_data);
  pool = (ABT_pool *)malloc(1 * sizeof(ABT_pool));
  ABT_sched_get_pools(sched, 1, 0, pool);
  ABT_pool_get_data(pool[0], (void **)&p_pool);
  
  while (1) {

    if(p_pool->active==0) {

      pthread_mutex_lock(&p_pool->active_lock);
      pthread_cond_wait(&p_pool->cond, &p_pool->active_lock);
      pthread_mutex_unlock(&p_pool->active_lock); //TEST
     //continue;
    }



    ABT_thread thread;
    //ABT_unit unit;
    //ABT_pool_pop(pool[0], &unit);
    //ABT_unit_get_thread(unit, &thread);

    //if (thread != ABT_THREAD_NULL) {

    
    int pop_stat = ABT_pool_pop_thread(pool[0], &thread);
    assert(pop_stat == ABT_SUCCESS);
    
    if (pop_stat == ABT_SUCCESS && thread != ABT_THREAD_NULL) {

      
    
      ABT_self_schedule(thread, pool[0]);

    } else {
      target_pool = rand() % streams;

     
      
      /*
      ABT_pool_pop(pools[target_pool], &unit);
      ABT_unit_get_thread(unit, &thread);
      */
      if ((ABT_pool_pop_thread_ex(pools[target_pool], &thread,
                                      ABT_POOL_CONTEXT_OWNER_SECONDARY) ==
               ABT_SUCCESS) &&
              (thread != ABT_THREAD_NULL)) {
             

        //ABT_pool_push(pool[0], unit);
        p_pool->task_stolen+=1;
        ABT_self_schedule(thread, pool[0]);
      
      }

      
    }

    if (finish == 1) {
      
      assert(ABT_sched_exit(sched)==ABT_SUCCESS);
      
      
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
#ifdef DEBUG
  ABT_unit_id thread_id;
  assert(ABT_thread_get_id(thread, &thread_id) == ABT_SUCCESS);
  printf("Created ThreadId: %ld pool: %d\n", thread_id, get_user_pool_rank(pool));
#endif
  return (ABT_unit)p_unit;
}

void pool_free_unit_1(ABT_pool pool, ABT_unit unit) {
  unit_t *p_unit = (unit_t *)unit;
#ifdef DEBUG
  ABT_unit_id thread_id;
  assert(ABT_thread_get_id(p_unit->thread, &thread_id) == ABT_SUCCESS);
  printf("Finished ThreadId: %ld pool: %d\n", thread_id, get_user_pool_rank(pool));
#endif
  free(p_unit);
}

ABT_bool pool_is_empty_1(ABT_pool pool) {
  pool_energy_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  return p_pool->p_head ? ABT_FALSE : ABT_TRUE;
}

ABT_thread pool_pop_1(ABT_pool pool, ABT_pool_context tail) {

  pool_energy_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  unit_t *p_unit = NULL;
  int own_pool = !(tail == ABT_POOL_CONTEXT_OWNER_SECONDARY);
  
  //if (!(tail & ABT_POOL_CONTEXT_OWNER_SECONDARY)) {
  if (own_pool) {
	  // TODO: Remove it after fixing crash issue
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
	// TODO: Set granularity after fixing crash
    pthread_mutex_lock(&p_pool->lock);

    // If only one thread or no thread
    if (p_pool->p_head == NULL) {
      pthread_mutex_unlock(&p_pool->lock);
      return ABT_THREAD_NULL;
     }
    if (p_pool->p_head == p_pool->p_tail) {
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
  return p_unit->thread;
}

void pool_push_1(ABT_pool pool, ABT_unit unit, ABT_pool_context c) {
  pool_energy_t *p_pool;
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
  pool_energy_t *p_pool = (pool_energy_t *)calloc(1, sizeof(pool_energy_t));
  if (!p_pool)
    return ABT_ERR_MEM;

  int ret = pthread_mutex_init(&p_pool->lock, 0);
  int ret2= pthread_mutex_init(&p_pool->active_lock,0);
  int ret3 = pthread_cond_init(&p_pool->cond, 0);
  if (ret != 0 || ret2!=0 || ret3!=0) {
    free(p_pool);
    return ABT_ERR_SYS;
  }
  p_pool->active=1;
  ABT_pool_set_data(pool, (void *)p_pool);
  return ABT_SUCCESS;
}

void pool_free_1(ABT_pool pool) {
  pool_energy_t *p_pool;
  ABT_pool_get_data(pool, (void **)&p_pool);
  pthread_mutex_destroy(&p_pool->lock);
  pthread_mutex_destroy(&p_pool->active_lock);
  pthread_cond_destroy(&p_pool->cond);
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
