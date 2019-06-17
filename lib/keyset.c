#define __LIBRARY__
#include <unistd.h>

_syscall2(int,keyset,const char*,key,int,glob_flag)
