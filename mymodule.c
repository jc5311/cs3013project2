// We need to define __KERNEL__ and MODULE to be in Kernel space
// If they are defined, undefined them and define them again:
#undef __KERNEL__
#undef MODULE

#define __KERNEL__ 
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <asm-generic/cputime.h>
#include "processinfo.h"

unsigned long **sys_call_table;
//struct processinfo *pinfo; //should probably delete this

//asmlinkage long (*ref_sys_cs3013_syscall1)(void);
//asmlinkage long (*ref_sys_open)(const char __user *filename, int flags, umode_t mode);
//asmlinkage long (*ref_sys_close)(unsigned int fd);
asmlinkage long (*ref_sys_cs3013_syscall2)(void);

/*asmlinkage long new_sys_cs3013_syscall1(void) {
    printk(KERN_INFO "\"'Hello world?!' More like 'Goodbye, world!' EXTERMINATE!\" -- Dalek");
    return 0;
}*/

/*asmlinkage long new_sys_open(const char __user *filename, int flags, umode_t mode){
	uid_t uid = current_uid().val;
	if (uid >= 1000)
		printk(KERN_INFO "User %d is opening file: %s\n", uid, filename);
	return ref_sys_open(filename, flags, mode);
}*/

/*asmlinkage long new_sys_close(unsigned int fd){
	uid_t uid = current_uid().val;
	if (uid >= 1000)
		printk(KERN_INFO "User %d is closing file descriptor: %d\n", uid, fd);
	return ref_sys_close(fd);
}*/

asmlinkage long cs3013_syscall2(struct processinfo *pinfo){
  struct task_struct *task = current;
  struct processinfo func_pinfo; //never blindly follow pointers! Create a temporary process info struct to fill.
  printk(KERN_INFO "current process: %s, PID: %d", task->comm, task->pid);
  
  //fill in simple data
  func_pinfo.state = task->state;
  func_pinfo.pid = task->pid;
  func_pinfo.parent_pid = task->parent->pid;

  //gather child and sibling information
  //begin with finding the youngest child
  if (list_empty(&task->children)){
    //no children
    func_pinfo.youngest_child = -1;
  } else{
    //the youngest child is not guaranteed to be the last entry
    //the youngest child should be the one with the latest start_time
    //func_pinfo->youngest_child = list_last_entry(&task->children, struct task_struct, children)->pid;
    
    struct list_head *p;
    static LIST_HEAD(child_list);
    struct task_struct *c;
    struct task_struct *youngest;
    youngest->start_time = 0;

    //loop through children list. Keep track of which has the latest start_time
    list_for_each(p, &child_list){
      c = list_entry(p, struct task_struct, children);
      if((c->start_time) > youngest->start_time){
        youngest = c;
      }
    }
    func_pinfo.youngest_child = youngest->pid;
  }

  //find younger sibling
  //loop through the sibling list
  //find which has a start_time larger than the current process and is closer to it
  if (list_empty(&task->sibling)){
    func_pinfo.younger_sibling = -1;
  }
  else{
    struct list_head *p;
    struct task_struct *s;
    struct task_struct *younger_sibling;
    static LIST_HEAD(sibling_list);
    u64 time_difference;
    u64 reference_time = task->start_time;
    u64 shortest_time = reference_time;

    list_for_each(p, &sibling_list){
      s = list_entry(p, struct task_struct, sibling);
      time_difference = s->start_time - reference_time;
      if ((time_difference > 0) && (time_difference < shortest_time)){
        shortest_time = time_difference;
        younger_sibling = s;
      }
    }
    func_pinfo.younger_sibling = younger_sibling->pid;
  }

  //find older sibling
  //loop through the sibling lst
  //find which has a start time smaller than the current process and is closer to it
  if (list_empty(&task->sibling)){
    func_pinfo.older_sibling = -1;
  }
  else{
    struct list_head *p;
    struct task_struct *s;
    struct task_struct *older_sibling;
    static LIST_HEAD(sibling_list);
    u64 time_difference;
    u64 reference_time = task->start_time;
    u64 shortest_time = reference_time;

    list_for_each(p, &sibling_list){
      s = list_entry(p, struct task_struct, sibling);
      time_difference = s->start_time - reference_time;
      if ((time_difference < 0) && (time_difference < shortest_time)){
        shortest_time = time_difference;
        older_sibling = s;
      }
    }
    func_pinfo.older_sibling = older_sibling->pid;
  }

  //start filling in the rest of the data
  func_pinfo.uid = task->cred->uid.val;
  func_pinfo.start_time = task->start_time; //there doesn't look to be a timespec var...
  func_pinfo.user_time = cputime_to_usecs(task->utime);
  func_pinfo.sys_time = cputime_to_usecs(task->stime);
  


  return 0;
}


static unsigned long **find_sys_call_table(void) {
	//this function...finds the sys_call table
	//do not edit
  unsigned long int offset = PAGE_OFFSET;
  unsigned long **sct;
  
  while (offset < ULLONG_MAX) {
    sct = (unsigned long **)offset;

    if (sct[__NR_close] == (unsigned long *) sys_close) {
      printk(KERN_INFO "Interceptor: Found syscall table at address: 0x%02lX",
                (unsigned long) sct);
      return sct;
    }
    
    offset += sizeof(void *);
  }
  
  return NULL;
}

static void disable_page_protection(void) {
  /*
    Control Register 0 (cr0) governs how the CPU operates.

    Bit #16, if set, prevents the CPU from writing to memory marked as
    read only. Well, our system call table meets that description.
    But, we can simply turn off this bit in cr0 to allow us to make
    changes. We read in the current value of the register (32 or 64
    bits wide), and AND that with a value where all bits are 0 except
    the 16th bit (using a negation operation), causing the write_cr0
    value to have the 16th bit cleared (with all other bits staying
    the same. We will thus be able to write to the protected memory.

    It's good to be the kernel!
   */
  write_cr0 (read_cr0 () & (~ 0x10000));
}

static void enable_page_protection(void) {
  /*
   See the above description for cr0. Here, we use an OR to set the 
   16th bit to re-enable write protection on the CPU.
  */
  write_cr0 (read_cr0 () | 0x10000);
}

static int __init interceptor_start(void) {
  /* Find the system call table */
  if(!(sys_call_table = find_sys_call_table())) {
    /* Well, that didn't work. 
       Cancel the module loading step. */
    return -1;
  }
  
  /* Store a copy of all the existing functions */
  //ref_sys_cs3013_syscall1 = (void *)sys_call_table[__NR_cs3013_syscall1];
  //ref_sys_open = (void*)sys_call_table[__NR_open];
  //ref_sys_close = (void*)sys_call_table[__NR_close];
  ref_sys_cs3013_syscall2 = (void *)sys_call_table[__NR_cs3013_syscall2];

  /* Replace the existing system calls */
  disable_page_protection();

  //sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)new_sys_cs3013_syscall1;
  //sys_call_table[__NR_open] = (unsigned long *)new_sys_open;
  //sys_call_table[__NR_close] = (unsigned long *)new_sys_close;
  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)cs3013_syscall2;
  
  enable_page_protection();
  
  /* And indicate the load was successful */
  printk(KERN_INFO "Loaded interceptor!");

  return 0;
}

static void __exit interceptor_end(void) {
  /* If we don't know what the syscall table is, don't bother. */
  if(!sys_call_table)
    return;
  
  /* Revert all system calls to what they were before we began. */
  disable_page_protection();
  //sys_call_table[__NR_cs3013_syscall1] = (unsigned long *)ref_sys_cs3013_syscall1;
  //sys_call_table[__NR_open] = (unsigned long *)ref_sys_open;
  //sys_call_table[__NR_close] = (unsigned long *)ref_sys_close;
  sys_call_table[__NR_cs3013_syscall2] = (unsigned long *)ref_sys_cs3013_syscall2;
  enable_page_protection();

  printk(KERN_INFO "Unloaded interceptor!");
}

MODULE_LICENSE("GPL");
module_init(interceptor_start);
module_exit(interceptor_end);
