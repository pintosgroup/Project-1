#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/* Process identifier. */ // (Project 2)
typedef int pid_t;
#define PID_ERROR ((pid_t) -1)

void syscall_init (void);

// Exit must be called in exception (Project 2)
void sys_exit (int status);

#endif /* userprog/syscall.h */
