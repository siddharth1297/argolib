/*
 * Header file for argolib C interfaces.
 */
#include "argolib.h"
/*
int streams = 0;
ABT_xstream *xstreams = NULL;
ABT_pool* pools = NULL;
ABT_sched *scheds = 0;
*/

/**
 * Initializes the ArgoLib runtime, and it should be the first thing to call in
 * the user main. Arguments “argc” and “argv” are the ones passed in the call to
 * user main method.
 */
void argolib_init(int argc, char **argv) {

  ABT_init(argc, argv);

  streams = 0;
  if (streams <= 0)
    streams = 1;

  /* Allocate memory. */
  /*
  ABT_xstream *xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * streams);
  ABT_pool* pools = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);
  ABT_sched *scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * streams);
	*/
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * streams);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);
  scheds = (ABT_sched *)malloc(sizeof(ABT_sched) * streams);

  // Create pools
  for (int i = 0; i < streams; i++)
    ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE,
                          &pools[i]);

  // Create Schedulers
  for (int i = 0; i < streams; i++) {

    ABT_pool *tmp = (ABT_pool *)malloc(sizeof(ABT_pool) * streams);

    for (int j = 0; j < streams; j++)
      tmp[j] = pools[(i + j) % streams];

    ABT_sched_create_basic(ABT_SCHED_RANDWS, streams, tmp,
                           ABT_SCHED_CONFIG_NULL, &scheds[i]);
    free(tmp);
  }

  /* Set up a primary execution stream. */
  ABT_xstream_self(&xstreams[0]);
  ABT_xstream_set_main_sched(xstreams[0], scheds[0]);

  /* Create secondary execution streams. */
  for (int i = 1; i < streams; i++) {
    ABT_xstream_create(scheds[i], &xstreams[i]);
  }
}

/**
 * Finalize the ArgoLib runtime, and performs the cleanup.
 */
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

/**
 * User can use this API to launch the top-level computation kernel. This API
 * gathers statistics for the parallel execution and reports after the
 * completion of the computation kernel. Some of the statistics are: execution
 * time, total tasks created, etc. This top-level kernel would actually be
 * launching the recursive tasks.
 */
void argolib_kernel(fork_t fptr, void *args) {

  fptr(args);
  printf("Task count :%d\n", counter);
}

/**
 * Creates an Argobot ULT that would execute a user method with the specified
 * argument. It returns a pointer to the task handle that would be used for
 * joining this ULT.
 */
Task_handle argolib_fork(fork_t fptr, void *args) {

  counter++;

  int rank;
  ABT_xstream_self_rank(&rank);
  ABT_pool target_pool = pools[rank];
  ABT_thread child;

  int x =
      ABT_thread_create(target_pool, fptr, args, ABT_THREAD_ATTR_NULL, &child);

  return (child);
}

/**
 * Used for joining one more ULTs using the corresponding task handles. In case
 * of more than one task handles, user can pass an array of Task_handle*. The
 * parameter “size” is the array size.
 */
void argolib_join(Task_handle list[], int size) {

  for (int i = 0; i < size; i++) {

    int x = ABT_thread_join(list[i]);
    int y = ABT_thread_free(&list[i]);
  }
}
