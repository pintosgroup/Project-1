#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

// Struct for keeping track of process execution data (Project 2)
struct exec_info
{
  char * file_name; // Command passed in with arguments
  struct semaphore load_done; // Wait for child to finish loading
  bool success; // Successful load of child
  struct wait_info *wait_status; // Wait info so the parent can keep track of its children
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
