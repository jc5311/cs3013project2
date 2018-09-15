#ifndef PROCCESSINFO_H
#define PROCCESSINFO_H

struct processinfo{
	long state; //current state of process
	pid_t pid; //process ID of this process
	pid_t parent_pid; //process ID of parent
	pid_t youngest_child; //process ID of youngest child
	pid_t younger_sibling; //pid of next younger sibling
	pid_t older_sibling; //pid of next older sibling
	uid_t uid; //user ID of process owner
	long long start_time; //process start time in ns since boot time
	long long user_time; //CPU time in user mode (us)
	long long sys_time; //CPU time in system mode (us)
	long long cutime; //user time of children (us)
	long long cstime; //system time of children (us)
}; //struct process info

#endif //PROCESSINFO_H
