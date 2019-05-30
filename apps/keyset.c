#include <unistd.h>
#include <string.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){

    if(get_argc(args) != 2){
        printerr("error: invalid number of arguments\n");
        printstr("Usage:\n keyset <key>\n");
        _exit(1);
    }
    char new_key[1024];
    strcpy(new_key, get_argv(args, 1));

    keyset(new_key);
    
    _exit(0);
}