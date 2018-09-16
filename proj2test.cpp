//Team: Juan Caraballo

/*
* The purpose of this program is to test a pair of intercepting functions
* created by the LKM mymodule.ko. The first intercepted function 
* __NR_cs3013_syscall1 should return 0 here if functioning properly.
* In the case of the interception for __NR_cs3013_syscall2 it should return
* a struct named processinfo that contains data about a running process.
*
* To test this second interception multiple tests will be made:
*
* Test Parent process
* The first test will call syscall2 and print the data returned from it.
* Because no forking has occured yet the values for youngest_child,
* younger_sibling, and older_sibling should return as -1. Similarly the values
* for cutime and cstime should return as 0.
*
* Test Children Processes
* To ensure that proper forking has occured and that the running process
* correctly has children we will perform two forks. Within each forked process
* we will print the pid's of their older and younger sibling. For the younger
* sibling, when we print the pid of the older sibling it should match the pid
* printed for the older sibling. Similarly when we print the pid of the younger
* sibling from the older sibling process, we should see the same printed pid for
* the younger process.
*/

#include <iostream>
#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include "processinfo_user.h"

// These values MUST match the syscall_32.tbl modifications:
#define __NR_cs3013_syscall1 377
#define __NR_cs3013_syscall2 378

long testCall1 ( void) {
    return (long) syscall(__NR_cs3013_syscall1);
}
long testCall2 ( void) {
    return (long) syscall(__NR_cs3013_syscall2);
}

int main () {
    //test sys call 1 for confirmation
    cout << "The return value of syscall1 is: " << testCall1() << endl;

    //begin test 1
    cout << "Beginning test 1 for syscall2." << endl;







    return 0;
}
