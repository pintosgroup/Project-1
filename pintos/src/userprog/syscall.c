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
#include "userprog/pagedir.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);
static char * copy_in_string (const char *);
static inline bool get_user (uint8_t *dst, const uint8_t *usrc);
static void sys_halt (void);
static pid_t sys_exec (const char *);
static int sys_wait (pid_t pid);
static bool sys_create(const char *file, unsigned initial_size);
static bool sys_remove(const char *file);
static int sys_open (const char * ufile);
static int sys_filesize (int fd);
static int sys_read (int fd, void *buffer, unsigned size);
static int sys_write (int fd, const void *, unsigned size);
static void sys_seek (int fd, unsigned position);
static unsigned sys_tell (int fd);
static void sys_close (int fd);

// Filesystem lock (Project 2)
struct lock fs_lock;

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  // Initialize the lock (Project 2)
  lock_init(&fs_lock);
}

static void
syscall_handler (struct intr_frame *f)
{
  void *args[3];
  // Get the system call number off the stack (Project 2)
  int sys_num = *(int *)f->esp;
  // Variables for system calls (Project 2)
  args[0] = (void *)f->esp + 4;
  args[1] = (void *)f->esp + 8;
  args[2] = (void *)f->esp + 12;

  // Make sure the arguments are not in kernel space (Project 2)
  if(is_kernel_vaddr(args[2]))
  {
    sys_exit(-1);
  }

  // Make the correct system call (Project 2)
  switch (sys_num) {
    case SYS_HALT: // 0
      sys_halt();
      break;
    case SYS_EXIT: // 1
      sys_exit(*(int *)args[0]);
      break;
    case SYS_EXEC: // 2
      f->eax = sys_exec(*(char **)args[0]);
      break;
    case SYS_WAIT: // 3
      f->eax = sys_wait(*(pid_t *)args[0]);
      break;
    case SYS_CREATE: // 4
      f->eax = sys_create(*(char **)args[0], *(unsigned *)args[1]);
      break;
    case SYS_REMOVE: // 5
      f->eax = sys_remove(*(char **)args[0]);
      break;
    case SYS_OPEN: // 6
      f->eax = sys_open(*(char **) args[0]);
      break;
    case SYS_FILESIZE: // 7
      f->eax = sys_filesize(*(int *)args[0]);
      break;
    case SYS_READ: // 8
      f->eax = sys_read(*(int *)args[0], *(void **)args[1], *(unsigned *)args[2]);
      break;
    case SYS_WRITE: // 9
      f->eax = sys_write(*(int *)args[0], *(void **)args[1], *(unsigned *)args[2]);
      break;
    case SYS_SEEK: // 10
      sys_seek(*(int *)args[0], *(unsigned *)args[1]);
      break;
    case SYS_TELL: // 11
      f->eax = sys_tell(*(int *)args[0]);
      break;
    case SYS_CLOSE: // 12
      sys_close(*(int *)args[0]);
      break;
    default:
      thread_exit();
      break;
  }
}

// Copy a string from user space into kernel space (Project 2)
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
      if (is_kernel_vaddr(us) || !get_user ((uint8_t *)(ks + length), (uint8_t *)(us++)))
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

// Copy a byte from one address to another (Project 2)
static inline bool
get_user (uint8_t *dst, const uint8_t *usrc)
{
  int eax;
  asm ("movl $1f, %%eax; movb %2, %%al; movb %%al, %0; 1:"
       : "=m" (*dst), "=&a" (eax) : "m" (*usrc));
  return eax != 0;
}

// Shutdown when called (Project 2)
static void
sys_halt (void)
{
  shutdown_power_off();
}

// Exit thread with called (Project 2)
void
sys_exit (int status)
{
  struct thread *cur = thread_current();

  // Set up exit info before exiting
  cur->exit_status = status;
  cur->wait_status->exit_code = status;
  cur->wait_status->ref_cnt--;

  // Signal parent that the child has finished
  sema_up(&cur->p_done);

  // Print exit information
  printf("%s: exit(%d)\n", cur->name, status);

  // sys_close all files associated with thread before exit
  lock_acquire(&fs_lock);
  struct list_elem *e;
  struct file_descriptor *file_d;

  if (!list_empty(&cur->fd_list)) {
    for (e = list_begin (&cur->fd_list); e != list_end (&cur->fd_list); e = list_remove(e))
    {
      file_d = list_entry (e, struct file_descriptor, elem);
      file_close(file_d->file);
    }
  }
  lock_release(&fs_lock);

  thread_exit();
}

// Execute with given arguments (Project 2)
static pid_t
sys_exec (const char *cmd_line)
{
  pid_t ret_val;

  // Make sure argument is in user space
  if (is_kernel_vaddr(cmd_line)) {
    return -1;
  }

  // Copy over command line into kernel space
  char *kcmd_line = copy_in_string (cmd_line);
  // Execute process
  lock_acquire (&fs_lock);
  ret_val = process_execute(kcmd_line);
  lock_release (&fs_lock);
  return ret_val;
}

// Wait on given process (Project 2)
static int
sys_wait (pid_t pid)
{
  return process_wait(pid);
}

// Create a file but do not open (Project 2)
static bool
sys_create(const char *file, unsigned initial_size)
{
  bool returnValue = false;
  if (file != NULL){
    char *kfile = copy_in_string (file);\
  
    if (kfile == NULL){
      sys_exit(-1);
    }
    else{
      // Lock the file system before calling create.
      lock_acquire(&fs_lock);
      returnValue = filesys_create(kfile, initial_size);
      lock_release(&fs_lock);
    }
    palloc_free_page (kfile);
  }else{
    sys_exit(-1);
  }
  return returnValue;
}

// Remove the file from the file system (Project 2)
static bool
sys_remove(const char *file)
{
  bool returnValue = false;
  if (file != NULL){
    char *kfile = copy_in_string (file);
    if (kfile == NULL){
      sys_exit(-1);
    }
    else{
      // Lock the file system before calling create. (Kevin)
      lock_acquire(&fs_lock);
      returnValue = filesys_remove(kfile);
      lock_release(&fs_lock);
    }
    palloc_free_page (kfile);
  }else{
    sys_exit(-1);
  }
  return returnValue;
}

// Opens a file and adds it to the threads list of open files (Project 2)
static int
sys_open(const char* ufile)
{
  char *kfile = copy_in_string (ufile);
  struct file_descriptor *fd;
  int handle = -1;

  fd = malloc(sizeof *fd);
  if (fd != NULL)
  {
      // Open the file and add it to the file list
      lock_acquire (&fs_lock);
      fd->file = filesys_open (kfile);
      if (fd->file != NULL)
      {
          struct thread *cur = thread_current ();
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

// Gets the size of the give file. Returns 0 if file not present (Project 2)
static int
sys_filesize (int fd)
{
  int ret_val = 0;
  struct file_descriptor *file_d;
  lock_acquire (&fs_lock);
  file_d = get_fd(fd);
  if (file_d != NULL) {
    ret_val = file_length(file_d->file);
  }
  lock_release (&fs_lock);
  return ret_val;
}

// Read for the given size. Returns -1 if read fails (Project 2)
static int
sys_read (int fd, void *buffer, unsigned size)
{
  int ret_val = -1;
  unsigned i;
  
  // Exit if reading from kernel space
  if (is_kernel_vaddr(buffer)) {
    sys_exit(-1);
  }

  // Read from console
  if (fd == 0) {
    for (i = 0; i < size; i++) {
      *(char *)(buffer+i) = (char) input_getc();
    }
    ret_val = i + 1;
  }
  // Read from file
  else {
    struct file_descriptor *file_d;
    lock_acquire (&fs_lock);
    file_d = get_fd(fd);
    if (file_d != NULL) {
      ret_val = file_read(file_d->file, buffer, (off_t)size);
    }
    lock_release (&fs_lock);
  }
  return ret_val;
}

// Write for the given size. Returns 0 if write fails (Project 2)
static int
sys_write (int fd, const void *buffer, unsigned size)
{
  int ret_val = 0;
  
  // Exit if writing to kernel space
  if (is_kernel_vaddr(buffer)) {
    sys_exit(-1);
  }

  char *kbuffer = copy_in_string (buffer);

  // If writing to console use putbuf
  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  // Write to file
  else {
    struct file_descriptor *file_d;
    lock_acquire (&fs_lock);
    file_d = get_fd(fd);
    if (file_d != NULL) {
      ret_val = file_write(file_d->file, buffer, (off_t)size);
    }

    lock_release (&fs_lock);
  }
  return ret_val;
}

// Seek to desired postion (Project 2)
static void
sys_seek (int fd, unsigned position)
{
  struct file_descriptor *file_d;
  lock_acquire (&fs_lock);
  file_d = get_fd(fd);
  if (file_d != NULL) {
    file_seek(file_d->file, (off_t)position);
  }
  lock_release (&fs_lock);
}

// Return the next position in the file (Project 2)
static unsigned
sys_tell (int fd)
{
  unsigned ret_val = 0;
  
  struct file_descriptor *file_d;
  lock_acquire (&fs_lock);
  file_d = get_fd(fd);
  if (file_d != NULL) {
    ret_val = file_tell(file_d->file);
  }
  lock_release (&fs_lock);
  
  return ret_val;
}

// Close the file and release resources (Project 2)
static void
sys_close (int fd)
{
  struct file_descriptor *file_d;
  lock_acquire (&fs_lock);
  file_d = get_fd(fd);
  if (file_d != NULL) {
    file_close(file_d->file);
    list_remove(&file_d->elem);
  }
  lock_release (&fs_lock);
  free(file_d);
}

