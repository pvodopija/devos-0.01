#define __LIBRARY__
#include <unistd.h>

_syscall2(int,stty,int,tty_index,long,settings)
