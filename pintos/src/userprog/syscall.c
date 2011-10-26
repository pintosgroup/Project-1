#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);
static void exit (int status);
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
  // Variables for system calls (Jim)
  args[0] = (void *)f->esp + 4;
  args[1] = (void *)f->esp + 8;
  args[2] = (void *)f->esp + 12;

  switch (sys_num) {
    case SYS_EXIT:  // 1
      exit(*(int *)args[0]);
      break;
    case SYS_WRITE: // 9
      write(*(int *)args[0], *(void **)args[1], *(unsigned *)args[2]);
      break;
    default:
      thread_exit();
      break;
  }
}

static void
exit (int status)
{
  // Singal parent to continue (Jim)
  sema_up(&thread_current()->p_done);
  // Print exit information and quit (Jim)
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
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

