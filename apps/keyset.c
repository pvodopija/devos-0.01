#include <unistd.h>
#include <string.h>
#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(void){
    char new_key[1024];

    stty(CONSOLE, N_ECHO);  /* disable printing to console setting */
    int key_len = read(STDIN_FILENO, new_key, sizeof(new_key));
    stty(CONSOLE, ECHO);    /* enable printing to console setting */

    new_key[key_len-1] = '\0';

    int status = keyset(new_key, K_GLOBAL);

    _exit(status);
}