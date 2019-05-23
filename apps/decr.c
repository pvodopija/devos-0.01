#include <unistd.h>
#include <string.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* arg){

    char file_name[50];
    if(get_argc(arg) != 2){
        printerr("error: invalid number of arguments\n");
        _exit(1);
    }

    strcpy(file_name, get_argv(arg, 1));
    
    decr(file_name);
    
    _exit(0);
}