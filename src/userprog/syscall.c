#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include <string.h>
#include "userprog/process.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "devices/input.h"

struct fd2file{
  struct list_elem elem;
  int fd;
  struct file *file;
};

struct semaphore filesys_lock;

static void syscall_handler (struct intr_frame *);
static void sys_halt(void);
static void sys_exit(int args[]);
static int sys_exec(int args[]);
static int sys_wait(int args[]);
static bool sys_create(int args[]);
static bool sys_remove(int args[]);
static int sys_open(int args[]);
static int sys_filesize(int args[]);
static int sys_read(int args[]);
static int sys_write(int args[]);
static void sys_seek(int args[]);
static unsigned sys_tell(int args[]);
static void sys_close(int args[]);
static void check_address(void *addr);
static void check_addressNULL(void *addr);
static struct fd2file *getfile(int fd);
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  sema_init(&filesys_lock,1);
}

static void
syscall_handler (struct intr_frame *f) 
{
  check_addressNULL((void *)f->esp);
  check_address((char *)f->esp+sizeof(struct intr_frame));
  
  
  /* get the syscall number and arguments */
  int tmp[4];
  memcpy(tmp,f->esp,sizeof(int)*4);
  int number=tmp[0];
  int *args=tmp+1;
  switch (number){
  case SYS_HALT:
    sys_halt();
    break;
  case SYS_EXIT:
    sys_exit(args);
    break;
  case SYS_EXEC:
    f->eax=(unsigned int)sys_exec(args);
    break;
  case SYS_WAIT:
    f->eax=(unsigned int)sys_wait(args);
    break;
  case SYS_CREATE:
    f->eax=(unsigned int)sys_create(args);
    break;
  case SYS_REMOVE:
    f->eax=(unsigned int)sys_remove(args);
    break;
  case SYS_OPEN:
    f->eax=(unsigned int)sys_open(args);
    break;
  case SYS_FILESIZE:
    f->eax=(unsigned int)sys_filesize(args);
    break;
  case SYS_READ:
    f->eax=(unsigned int)sys_read(args);
    break;
  case SYS_WRITE:
    f->eax=(unsigned int)sys_write(args);
    break;
  case SYS_SEEK:
    sys_seek(args);
    break;
  case SYS_TELL:
    f->eax=sys_tell(args);
    break;
  case SYS_CLOSE:
    sys_close(args);
    break;
  default:
    thread_exit ();
  }
}
/* if address is_kernel_vaddr terminate */
static void check_address(void * addr){
  if( is_kernel_vaddr(addr)==true){
    sys_exit_base(-1);
  }
}

/* if address is kernel vaddr or null, terminate */
static void check_addressNULL(void *addr){
  if(addr==NULL||is_kernel_vaddr(addr)==true||pagedir_get_page(thread_current()->pagedir,addr)==NULL){
    sys_exit_base(-1);
  }
}


static void sys_seek(int args[]){
  struct fd2file *tar=getfile(args[0]);
  if(tar==NULL){
    return;
  }
  sema_down(&filesys_lock);
  file_seek(tar->file,args[1]);
  sema_up(&filesys_lock);
}
static unsigned sys_tell(int args[]){
  struct fd2file *tar=getfile(args[0]);
  if(tar==NULL){
    return ~0;
  }
  sema_down(&filesys_lock);
  int tmp=file_tell(tar->file);
  sema_up(&filesys_lock);
  return tmp;
}
static void sys_close(int args[]){
  struct fd2file *tar=getfile(args[0]);
  if(tar==NULL){
    return;
  }
  sema_down(&filesys_lock);
  file_close(tar->file);
  sema_up(&filesys_lock);
  list_remove(&tar->elem);
  free(tar);
}
  
  
static int sys_open(int args[]){
  check_addressNULL((void*)args[0]);
  sema_down(&filesys_lock);
  struct file *file=filesys_open((char *)args[0]);
  sema_up(&filesys_lock);
  if(file==NULL){
    return -1;
  }
  struct fd2file *tt=malloc(sizeof(struct fd2file));
  tt->file=file;
  struct thread *cur=thread_current();
  tt->fd=list_entry(list_back(&cur->files),struct fd2file,elem)->fd+1;
  list_push_back(&cur->files,&tt->elem);
  return tt->fd;
}


static struct fd2file *getfile(int  fd){
  struct thread *cur=thread_current();
  struct fd2file *tar=NULL;
  for(struct list_elem *e=list_begin(&cur->files);e!=list_end(&cur->files);e=list_next(e)){
    struct fd2file *f=list_entry(e,struct fd2file,elem);
    if(f->fd==fd){
      tar=f;
      break;
    }
  }
  return tar;
}

static int sys_read(int args[]){
  check_addressNULL((void *)args[1]);
  check_address((char *)args[1]+args[2]);
  
  int len;
  if(args[0]==0){
    len=args[2];
    int index=0;
    char *buf=(char *)args[1];
    while(index<len){
      buf[index]=input_getc();
    }
    return len;
  }
  struct fd2file *tar=getfile(args[0]);
  if(tar==NULL){
    return -1;
  }
  sema_down(&filesys_lock);
  len=file_read(tar->file,(void *)args[1],args[2]);
  sema_up(&filesys_lock);
  return len;
}
    
    
static int sys_filesize(int args[]){
  if(args[0]<2){
    return -1;
  }
  struct fd2file *result=getfile(args[0]);
  if(result==NULL){
    return -1;
  }
  sema_down(&filesys_lock);
  int len=file_length(result->file);
  sema_up(&filesys_lock);
  return len;
}

static int sys_exec(int args[]){
  check_addressNULL((void *)args[0]);
  
  sema_down(&filesys_lock);
  int pid= process_execute((char *)args[0]);
  sema_up(&filesys_lock);
  return pid;
}
static bool sys_create(int args[]){  
  check_addressNULL((void *)args[0]);
  
  sema_down(&filesys_lock);
  bool result=filesys_create((char *)args[0],args[1]);
  sema_up(&filesys_lock);
  return result;
}
static bool sys_remove(int args[]){
  check_address((void *)args[0]);
  
  sema_down(&filesys_lock);
  bool result=filesys_remove((char *)args[0]);
  sema_up(&filesys_lock);
  return result;
}
static int sys_wait(int args[]){
  
  return process_wait(args[0]);
}
static void sys_exit(int args[]){ 
  sys_exit_base(args[0]);
}
void sys_exit_base(int tt){
  struct thread *cur=thread_current();
  printf("%s: exit(%d)\n",cur->name,tt);
  cur->exit_status=tt;
  sema_down(&filesys_lock);
  //free the memory resources has been allocated.
  while(list_empty(&cur->files)==false){
    struct fd2file *f=list_entry(list_pop_front(&cur->files),struct fd2file,elem);
    file_close(f->file);
    free(f);
  }
  sema_up(&filesys_lock);
  
  //as father
  for(struct list_elem *e=list_begin(&cur->sons);e!=list_end(&cur->sons);e=list_next(e)){
    struct thread *son=list_entry(e,struct thread,as_son);
    sema_up(&son->sema_wait);
  }
  sema_up(&cur->sema_wait);
  sema_down(&cur->sema_exit);
  //free locks
  while(list_empty(&cur->locks_holding)==false){
    list_pop_front(&cur->locks_holding);
  }
  //free virtual space
  thread_exit();
}
static int sys_write(int args[]){
  check_addressNULL((void*)args[1]);
  check_address((char *)args[1]+args[2]);
  
  int size;
  if(args[0]==1){
    size=args[2];
    putbuf((char *)args[1],size);
    return size;
  }
  struct fd2file *tar=getfile(args[0]);
  if(tar==NULL){
    return -1;
  }
  sema_down(&filesys_lock);
  size=file_write(tar->file,(void *)args[1],args[2]);
  sema_up(&filesys_lock);
  return size;
}
static void sys_halt(){
  shutdown_power_off();
}
