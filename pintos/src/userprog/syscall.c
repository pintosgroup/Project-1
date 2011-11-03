#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/init.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/palloc.h"

static void syscall_handler (struct intr_frame *);
static char * copy_in_string (const char *);
static inline bool get_user (uint8_t *dst, const uint8_t *usrc);
static void halt (void);
static void exit (int status);
static pid_t exec (const char *);
static int wait (pid_t pid);
static int open (const char * ufile);
static int write (int fd, const void *, unsigned size);

//filesystem lock
struct lock fs_lock;

//structure that describes file
struct file_descriptor
{
   struct list_elem elem; //list elem
   struct file *file; //file name
   //struct dir  *dir;  //file directory
   int handle;
};

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&fs_lock);
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
    case SYS_EXEC:  // 2
      f->eax = exec(*(char **)args[0]);
      break;
    case SYS_WAIT:  // 3
      f->eax = wait(*(pid_t *)args[0]);
      break;
    case SYS_OPEN:  //6
      f->eax = open(*(char **) args[0]);
      break;
    case SYS_WRITE: // 9
      f->eax = write(*(int *)args[0], *(void **)args[1], *(unsigned *)args[2]);
      break;
    default:
      thread_exit();
      break;
  }
}

/*static char *
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
}*/

static char*
copy_in_string (const char *us)
{
  char *ks;
  size_t length;
  ks = palloc_get_page (PAL_ASSERT | PAL_ZERO);
  if (ks == NULL)
      thread_exit ();
  for (length = 0; length < PGSIZE; length++)
  {
      if (us >= (char *) PHYS_BASE || !get_user (ks + length, us++))
      {
          palloc_free_page (ks);
          thread_exit ();
      }
      if (ks[length] == '\0')
          return ks;
  }
  ks[PGSIZE - 1] = '\0';
  return ks;
}


static inline bool
get_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
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
open(const char* ufile)
{
  char *kfile = copy_in_string (ufile);
  struct file_descriptor *fd;
  int handle = -1;

  fd = malloc(sizeof *fd);
  if (fd != NULL)
  {
      lock_acquire (&fs_lock);
      fd->file = filesys_open (kfile);
      //printf("File:  %s",fd->file);
      if (fd->file != NULL)
      {
          struct thread *cur = thread_current ();
          printf("Handle: %d\n", cur->next_handle);
          handle = fd->handle = cur->next_handle++;
          list_push_front (&cur->fd_list, &fd->elem);
      }
      else
      {
          free (fd);
      }
      lock_release (&fs_lock);
  }

  palloc_free_page (kfile);
  return handle;
}

static int
write (int fd, const void *buffer, unsigned size)
{

  char *kbuffer = copy_in_string (buffer);
  // If writing to console use putbuf (Jim)
  if (fd == 1) {
    putbuf(kbuffer, size);
  }

  return 0;
}

