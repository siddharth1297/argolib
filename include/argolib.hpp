/*
 * Header file for argolib CPP interfaces.
 */
#ifndef ARGOLIB_HPP
#define ARGOLIB_HPP

#include "argolibdefs.h"

#ifdef __cplusplus

#include <cstring>
#include <iostream>

extern "C" {
extern void argolib_init(int argc, char **argv);
extern void argolib_finalize();
extern void argolib_kernel(fork_t fptr, void *args);
extern Task_handle *argolib_fork(fork_t fptr, void *args);
extern void argolib_join(Task_handle **list, int size);
extern void argolib_start_tracing();
extern void argolib_stop_tracing();
}

namespace argolib {

/*
 * argolib cpp task structure
 */
typedef struct {
  void *args;
  fork_t _fp;
} argolib_cpp_task_t;

/*
 * Reffered from:
 * https://github.com/habanero-rice/hclib/blob/master/inc/hclib-async.h#L64 At
 * the lowest layer in the call stack before entering user code, this method
 * invokes the user-provided lambda.
 */
template <typename T> inline void call_lambda(T *lambda) {
  (*lambda)();
  delete lambda;
}

/*
 * Reffered from:
 * https://github.com/habanero-rice/hclib/blob/master/inc/hclib-async.h#L100
 * Store a reference to the type-specific function for calling the user lambda,
 * as well as a pointer to the lambda's location on the heap (through which we
 * can invoke it). async_arguments is stored as the args field in the task_t
 * object for a task, and passed to lambda_wrapper.
 */
template <typename Function, typename T1> struct async_arguments {
  Function lambda_caller;
  T1 *lambda_on_heap;
  async_arguments(Function k, T1 *a) : lambda_caller(k), lambda_on_heap(a) {}
};

/*
 * Reffered from:
 * https://github.com/habanero-rice/hclib/blob/master/inc/hclib-async.h#L114 The
 * method called directly from the HC runtime, passed a pointer to an
 * async_arguments object. It then uses these async_arguments to call
 * call_lambda, passing the user-provided lambda.
 */
template <typename Function, typename T1> void lambda_wrapper(void *args) {
  async_arguments<Function, T1> *a = (async_arguments<Function, T1> *)args;
  (*a->lambda_caller)(a->lambda_on_heap);
}

/*
 * Reffered from:
 * https://github.com/habanero-rice/hclib/blob/master/inc/hclib-async.h#L125
 * Initialize a task_t for the C++ APIs, using a user-provided lambda.
 */
template <typename Function, typename T1>
inline argolib_cpp_task_t *initialize_task(Function lambda_caller,
                                           T1 *lambda_on_heap) {
  argolib_cpp_task_t *t = (argolib_cpp_task_t *)calloc(1, sizeof(*t));
  assert(t && lambda_on_heap);
  async_arguments<Function, T1> *args =
      new async_arguments<Function, T1>(lambda_caller, lambda_on_heap);
  t->_fp = lambda_wrapper<Function, T1>;
  t->args = args;
  return t;
}

/**
 * C++ equivalent of the argolib_init API.
 */
void init(int argc, char **argv) { argolib_init(argc, argv); }

/**
 * C++ equivalent of the argolib_finalize API.
 */
void finalize() { argolib_finalize(); }

/*
 * Copied from:
 * https://github.com/habanero-rice/hclib/blob/master/inc/hclib-async.h#L140
 * lambda_on_heap is expected to be off-stack storage for a lambda object
 * (including its captured variables), which will be pointed to from the task_t.
 */
template <typename T> inline argolib_cpp_task_t *allocate_task(T *lambda) {
  T *lambda_on_heap = (T *)malloc(sizeof(*lambda_on_heap));
  assert(lambda_on_heap);
  memcpy(lambda_on_heap, lambda, sizeof(*lambda_on_heap));

  return initialize_task(call_lambda<T>, lambda_on_heap);
}

/**
 * C++ equivalent of the argolib_kernel API. User lambda passed instead of
 * function pointer and args.
 * https://stackoverflow.com/questions/115703/storing-c-template-function-definitions-in-a-cpp-file
 */
template <typename T> void kernel(T &&lambda) {
  typedef typename std::remove_reference<T>::type U;
  // argolib_cpp_task_t *task = allocate_task(new U(lambda));
  auto u = new U(lambda);
  argolib_cpp_task_t *task = allocate_task(u);
  fork_t fptr = (fork_t)((task->_fp));
  void *args = task->args;
  argolib_kernel(fptr, args);
  //free(args);
  //free(task);
  //delete u;
}

/**
 * C++ equivalent of the argolib_fork API. User lambda passed instead of
 * function pointer and args.
 */
template <typename T> Task_handle *fork(T &&lambda) {
  typedef typename std::remove_reference<T>::type U;
  // argolib_cpp_task_t *task = allocate_task(new U(lambda));
  auto u = new U(lambda);
  argolib_cpp_task_t *task = allocate_task(u);
  fork_t fptr = (fork_t)((task->_fp));
  void *args = task->args;
  Task_handle *ret = argolib_fork(fptr, args);
  //free(args);
  //free(task);
  //delete u;
  return ret;
}

/*
 * Helper method for join.
 * Not for public use
 */
template <typename T> void join_and_free(T handle) { argolib_join(&handle, 1); }

/*
 * Helper method for join.
 * Not for public use
 */
template <typename T, typename... Rest>
void join_and_free(T handle, Rest... rest) {
  argolib_join(&handle, 1);
  join_and_free(rest...);
}

/**
 * C++ equivalent of the argolib_join API.
 * All the task handles are passed as comma separated list. The three dots “…”
 * represents variable number of arguments.
 */
template <typename... T> void join(T... handles) {
  // https://kevinushey.github.io/blog/2016/01/27/introduction-to-c++-variadic-templates/
  join_and_free(handles...);
}

/**
 * C++ API for start tracing
 */
void start_tracing() { argolib_start_tracing(); }

/**
 * C++ API for stop tracing
 */
void stop_tracing() { argolib_stop_tracing(); }

} // namespace argolib

#else

#include "argolib.h"

#endif

#endif
