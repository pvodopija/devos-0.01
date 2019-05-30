#include <unistd.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(char* args){

   
    if(get_argc(args) != 2){
        printerr("error: invalid number of arguments\n");
        printstr("Usage:\n keygen <level>\n");
        _exit(1);
    }
    

    int level = atoi(get_argv(args, 1));
    keygen(level);


    _exit(0);
}