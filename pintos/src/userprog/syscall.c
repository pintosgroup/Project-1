#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
<<<<<<< HEAD
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"
#include "threads/init.h"
#include "threads/vaddr.h"
=======
>>>>>>> d5b4037... a few open tests pass

static void syscall_handler (struct intr_frame *);
static char * copy_in_string (const char *);
static void halt (void);
static void exit (int status);
static pid_t exec (const char *);
static int wait (pid_t pid);
static int write (int fd, const void *, unsigned size);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  void *args[3];
  // Get the system call number off the stack (Jim)
  int sys_num = *(int *)f->esp;
  //printf("sys_num = %d\n", sys_num);
  // Variables for system calls (Jim)
  args[0] = (void *)f->esp + 4;
  args[1] = (void *)f->esp + 8;
  args[2] = (void *)f->esp + 12;

  switch (sys_num) {
    case SYS_HALT:  // 0
      halt();
      break;
    case SYS_EXIT:  // 1
      exit(*(int *)args[0]);
      break;
<<<<<<< HEAD
    case SYS_OPEN:  //6
      f->eax = open((char *) args[0]);
    case SYS_EXEC:  // 2
      f->eax = exec(*(char **)args[0]);
      break;
    case SYS_WAIT:  // 3
      f->eax = wait(*(pid_t *)args[0]);
      break;
=======
>>>>>>> d5b4037... a few open tests pass
    case SYS_WRITE: // 9
      write(*(int *)args[0], *(void **)args[1], *(unsigned *)args[2]);
      break;
    default:
      thread_exit();
      break;
  }
}

static char *
copy_in_string (const char *us)
{
  char *ks;
  size_t length;
  ks = palloc_get_page (0);
  if (ks == NULL)
    thread_exit();
  for (length = 0; length < PGSIZE; length++)
  {
    ks[length] = us[length];
    if (ks[length] == '\0')
      return ks;
  }
  ks[PGSIZE - 1] = '\0';
  return ks;
}

static void
halt (void)
{
  shutdown_power_off();
}

static void
exit (int status)
{
  // Signal parent to continue (Jim)
  sema_up(&thread_current()->p_done);
  // Print exit information and quit (Jim)
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

static pid_t
exec (const char *cmd_line)
{
  char *kcmd_line = copy_in_string (cmd_line);
  return process_execute(kcmd_line);
}

static int
wait (pid_t pid)
{
  return process_wait(pid);
}

static int
write (int fd, const void *buffer, unsigned size)
{
  // If writing to console use putbuf (Jim)
  if (fd == 1) {
    putbuf(buffer, size);
  }

  return 0;
}

