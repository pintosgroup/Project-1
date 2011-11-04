#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

// Struct for keeping track of process execution data
struct exec_info
{
  char * file_name;
  struct semaphore load_done;
  bool success;
  struct thread *thread_created;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
