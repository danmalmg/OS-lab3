/*
 * Exercise on thread synchronization.
 *
 * Assume a half-duplex communication bus with limited capacity, measured in
 * tasks, and 2 priority levels:
 *
 * - tasks: A task signifies a unit of data communication over the bus
 *
 * - half-duplex: All tasks using the bus should have the same direction
 *
 * - limited capacity: There can be only 3 tasks using the bus at the same time.
 *                     In other words, the bus has only 3 slots.
 *
 *  - 2 priority levels: Priority tasks take precedence over non-priority tasks
 *
 *  Fill-in your code after the TODO comments
 */

#include <stdio.h>
#include <string.h>

#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/thread.h"
#include "timer.h"

/* This is where the API for the condition variables is defined */
#include "threads/synch.h"

/* This is the API for random number generation.
 * Random numbers are used to simulate a task's transfer duration
 */
#include "lib/random.h"

#define MAX_NUM_OF_TASKS 200

#define BUS_CAPACITY 3

typedef enum {
  SEND,
  RECEIVE,

  NUM_OF_DIRECTIONS
} direction_t;

typedef enum {
  NORMAL,
  PRIORITY,

  NUM_OF_PRIORITIES
} priority_t;

typedef struct {
  direction_t direction;
  priority_t priority;
  unsigned long transfer_duration;
} task_t;

void init_bus (void);
void batch_scheduler (unsigned int num_priority_send,
                      unsigned int num_priority_receive,
                      unsigned int num_tasks_send,
                      unsigned int num_tasks_receive);

/* Thread function for running a task: Gets a slot, transfers data and finally
 * releases slot */
static void run_task (void *task_);

/* WARNING: This function may suspend the calling thread, depending on slot
 * availability */
static void get_slot (const task_t *task);

/* Simulates transfering of data */
static void transfer_data (const task_t *task);

/* Releases the slot */
static void release_slot (const task_t *task);

static struct lock bus_lock;
static int slots;
static direction_t current_direction;
static int waiters_priority[2];
static int waiters_direction[2];
struct condition slot_cond[2];

void init_bus (void) {

  random_init ((unsigned int)123456789);

  cond_init(&slot_cond[0]);    //init condition variable for sending

  cond_init(&slot_cond[1]);    //init condition variable for receiving

  lock_init(&bus_lock);    //init bus lock

  slots = 0;    //slots on bus

  current_direction = SEND;    //init stating direction to SEND

  /*set waiting queues to empty*/
  waiters_priority[NORMAL] = 0;
  waiters_priority[PRIORITY] = 0;
  waiters_direction[NORMAL] = 0;
  waiters_direction[PRIORITY] = 0;
}

void batch_scheduler (unsigned int num_priority_send,
                      unsigned int num_priority_receive,
                      unsigned int num_tasks_send,
                      unsigned int num_tasks_receive) {
  ASSERT (num_tasks_send + num_tasks_receive + num_priority_send +
             num_priority_receive <= MAX_NUM_OF_TASKS);

  static task_t tasks[MAX_NUM_OF_TASKS] = {0};

  char thread_name[32] = {0};

  unsigned long total_transfer_dur = 0;

  int j = 0;

  /* create priority sender threads */
  for (unsigned i = 0; i < num_priority_send; i++) {
    tasks[j].direction = SEND;
    tasks[j].priority = PRIORITY;
    tasks[j].transfer_duration = random_ulong() % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "sender-prio");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* create priority receiver threads */
  for (unsigned i = 0; i < num_priority_receive; i++) {
    tasks[j].direction = RECEIVE;
    tasks[j].priority = PRIORITY;
    tasks[j].transfer_duration = random_ulong() % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "receiver-prio");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* create normal sender threads */
  for (unsigned i = 0; i < num_tasks_send; i++) {
    tasks[j].direction = SEND;
    tasks[j].priority = NORMAL;
    tasks[j].transfer_duration = random_ulong () % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "sender");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* create normal receiver threads */
  for (unsigned i = 0; i < num_tasks_receive; i++) {
    tasks[j].direction = RECEIVE;
    tasks[j].priority = NORMAL;
    tasks[j].transfer_duration = random_ulong() % 244;

    total_transfer_dur += tasks[j].transfer_duration;

    snprintf (thread_name, sizeof thread_name, "receiver");
    thread_create (thread_name, PRI_DEFAULT, run_task, (void *)&tasks[j]);

    j++;
  }

  /* Sleep until all tasks are complete */
  timer_sleep (2 * total_transfer_dur);
}

/* Thread function for the communication tasks */
void run_task(void *task_) {
  task_t *task = (task_t *)task_;

  get_slot (task);

  msg ("%s acquired slot", thread_name());
  transfer_data (task);

  release_slot (task);
}

static direction_t other_direction(direction_t this_direction) {
  return this_direction == SEND ? RECEIVE : SEND;
}

void get_slot (const task_t *task) {

  lock_acquire(&bus_lock);   
  /*if tasks can't get on bus, wait*/
  while ((slots == BUS_CAPACITY) ||    //if all slots are taken
   (slots > 0 && current_direction != task->direction ||    //if task is the wrong direction
    task->priority == NORMAL && waiters_priority[PRIORITY] > 0)) {    //if task is normal and priority tasks are waiting
    waiters_direction[task->direction]++;
    waiters_priority[task->priority]++;
    /*send task is waiting in queue*/
    if (task->direction == SEND) {
      cond_wait(&slot_cond[0], &bus_lock); 
    }
    /*receive task is waiting*/
    else {
      cond_wait(&slot_cond[1], &bus_lock);
    }
    /*remove task from queue once signaled*/
    waiters_direction[task->direction]--;
    waiters_priority[task->priority]--;
  }
  /*slot taken by task*/
  slots++;    
  current_direction = task->direction;    

  lock_release(&bus_lock);    
}

void transfer_data (const task_t *task) {
  /* Simulate bus send/receive */
  timer_sleep (task->transfer_duration);
}

void release_slot (const task_t *task) {

  lock_acquire(&bus_lock);
  
  /*free a slot*/
  slots--;    
  /*if all slots are free, wake up all tasks in the opposite direction*/
  if (slots == 0) {    
    direction_t opposite_direction = other_direction(task->direction);    
    cond_broadcast(&slot_cond[opposite_direction], &bus_lock);
    /*if there are priority tasks in the queue, signal one of them*/
  } else if (waiters_priority[PRIORITY] > 0) {    
    cond_signal(&slot_cond[current_direction], &bus_lock);    
    /*if there are no priority tasks, signal one normal task*/
  } else if (waiters_direction[current_direction] > 0) {   
    cond_signal(&slot_cond[current_direction], &bus_lock);    
  }

  lock_release(&bus_lock);   
}