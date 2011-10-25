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
  printf ("system call!\n");
  int sys_num = *(int *)f->esp;
  printf ("sys_num: %d\n", sys_num);
  switch (sys_num) {
    int status;
    int fd;
    void *buffer;
    unsigned size;
    case SYS_EXIT:  // 1
      status = *(((int *)f->esp) + 1);
      exit(status);
      break;
    /*case SYS_READ:  // 8
      fd = *(((int *)f->esp) + 1);
      buffer = (((void *)f->esp) + 2);
      size = *(((unsigned *)f->esp) + 3);
      read(fd, buffer, size);
      break;*/
    case SYS_WRITE: // 9
      fd = *(((int *)f->esp) + 1);
      //printf("fd: %d\n", fd);
      buffer = *(((int *)f->esp) + 2);
      //printf("buffer: 0x%x\n", buffer);
      size = *(((unsigned *)f->esp) + 3);
      //printf("size: %d\n", size);
      //hex_dump(buffer, buffer, 32, true);
      //hex_dump(f->esp, f->esp, 32, true);
      write(fd, buffer, size);
      break;
    default:
      thread_exit();
      break;
  }
  //thread_exit ();
}

static void
exit (int status)
{
  printf("Semaphore up (0x%x) for thread %d\n", &thread_current()->p_done, thread_current()->tid);
  sema_up(&thread_current()->p_done);
  printf("Exiting thread %d\n", thread_current()->tid);
  //thread_exit();
  process_exit();
  printf("Returned from exit\n");
  //thread_exit();
}

static int
write (int fd, const void *buffer, unsigned size)
{
  if (fd == 1) {
    putbuf(buffer, size);
  }

  return 0;
}

int read (int fd, void *buffer, unsigned size)
{
  if (fd == 0) {
    input_getc();
  }

  return 0;
}
