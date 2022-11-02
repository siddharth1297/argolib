/*
 * Header file for argolib TRACE/REPLAY list helper functions.
 */
#ifndef ARGOLIB_TR_LIST_H
#define ARGOLIB_TR_LIST_H

#include "argolibdefs.h"

trace_task_list_t *create_trace_data_node(counter_t task_id,
                                          int create_pool_idx,
                                          int exec_pool_idx,
                                          counter_t steal_count) {
  trace_task_list_t *node =
      (trace_task_list_t *)malloc(sizeof(trace_task_list_t));
  assert(node != NULL);
  node->prev = NULL;
  node->next = NULL;
  node->trace_data.task_id = task_id;
  node->trace_data.create_pool_idx = create_pool_idx;
  node->trace_data.exec_pool_idx = exec_pool_idx;
  node->trace_data.steal_count = steal_count;
  return node;
}

/*
 * Insert at head
 */
void add_to_trace_list(trace_task_list_t **list, trace_task_list_t *node) {
  if (*list == NULL) {
    *list = node;
#ifdef DEBUG
    printf("LIST init %p:: {%ld\t%d\t%d\t%ld}\n", list,
           node->trace_data.task_id, node->trace_data.create_pool_idx,
           node->trace_data.exec_pool_idx, node->trace_data.steal_count);
#endif
  } else {
#ifdef DEBUG
    // trace_task_list_t *olsLst = *list;
#endif
    node->next = *list;
    (*list)->prev = node;
    *list = node;
#ifdef DEBUG
    /*printf("LIST updt %p -> %p:: {%ld\t%d\t%d\t%ld}\n", olsLst, list,
           node->trace_data.task_id, node->trace_data.create_pool_idx,
           node->trace_data.exec_pool_idx, node->trace_data.steal_count);*/
#endif
  }
}

void add_trace_task_list_to_trace_list(trace_task_list_t **list,
                                       trace_task_list_t **node) {
  if (*list == NULL) {
    *list = *node;
  } else {
    (*node)->next = *list;
    (*list)->prev = *node;
    *list = *node;
  }
}

void remove_node_from_trace_list(trace_task_list_t **list,
                                 trace_task_list_t *curr) {
  trace_task_list_t *next = curr->next;
  if (next) {
    next->prev = curr->prev;
  }
  if (curr->prev)
    curr->prev->next = next;

  if (*list == curr) {
    *list = next;
  }
  curr->prev = curr->next = NULL;
}

/*
 * Reffiered from:
 * https://www.geeksforgeeks.org/insertion-sort-doubly-linked-list/
 */
void sortedInsert(trace_task_list_t **head_ref, trace_task_list_t *newNode) {
  trace_task_list_t *current;

  // if list is empty
  if (*head_ref == NULL)
    *head_ref = newNode;

  // if the node is to be inserted at the beginning
  // of the doubly linked list
  else if ((*head_ref)->trace_data.task_id >= newNode->trace_data.task_id) {
    newNode->next = *head_ref;
    newNode->next->prev = newNode;
    *head_ref = newNode;
  } else {
    current = *head_ref;
    // locate the node after which the new node
    // is to be inserted
    while (current->next != NULL &&
           current->next->trace_data.task_id < newNode->trace_data.task_id)
      current = current->next;

    /*Make the appropriate links */

    newNode->next = current->next;

    // if the new node is not inserted
    // at the end of the list
    if (current->next != NULL)
      newNode->next->prev = newNode;

    current->next = newNode;
    newNode->prev = current;
  }
}

/*
 * Reffiered from:
 * https://www.geeksforgeeks.org/insertion-sort-doubly-linked-list/
 */
void insertionSort(trace_task_list_t **head_ref) {
  // Initialize 'sorted' - a sorted doubly linked list
  trace_task_list_t *sorted = NULL;

  // Traverse the given doubly linked list and
  // insert every node to 'sorted'
  trace_task_list_t *current = *head_ref;
  while (current != NULL) {

    // Store next for next iteration
    trace_task_list_t *next = current->next;

    // removing all the links so as to create 'current'
    // as a new node for insertion
    current->prev = current->next = NULL;

    // insert current in 'sorted' doubly linked list
    sortedInsert(&sorted, current);

    // Update current
    current = next;
  }
  // Update head_ref to point to sorted doubly linked list
  *head_ref = sorted;
}

void freeList(trace_task_list_t *list) {
  trace_task_list_t *next = NULL;
  while (list) {
    next = list->next;
    free(list);
    list = next;
  }
}
#endif
