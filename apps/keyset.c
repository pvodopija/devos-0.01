#include <unistd.h>
#include <string.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* arg){

    if(get_argc(arg) != 2){
        printerr("error: invalid number of arguments\n");
        _exit(1);
    }

    char new_key[1024];
    strcpy(new_key, get_argv(arg, 1));

    keyset(new_key);
    
    _exit(0);
}