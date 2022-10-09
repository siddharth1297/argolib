#include <stdio.h>

#include "argolib.h"

typedef struct {

  int input;
  int output;

} thread_arg_t;

void fib(void *arg) {

  int n = ((thread_arg_t *)arg)->input;
  int *p_ret = &((thread_arg_t *)arg)->output;

  if (n < 2)
    *p_ret = n;

  else {

    thread_arg_t child1_arg = {n - 1, 0};
    thread_arg_t child2_arg = {n - 2, 0};

    Task_handle *t1 = argolib_fork(&fib, &child1_arg);

    Task_handle *t2 = argolib_fork(&fib, &child2_arg);

    Task_handle *arr[2];
    arr[0] = t1;
    arr[1] = t2;

    argolib_join(arr, 2);

    *p_ret = child1_arg.output + child2_arg.output;
  }
}

int main(int argc, char **argv) {

  int n = 10;

  argolib_init(argc, argv);

  thread_arg_t arg = {n, 0};

  argolib_kernel(&fib, &arg);

  argolib_finalize();

  printf("output is %d\n", arg.output);

  return 0;
}
