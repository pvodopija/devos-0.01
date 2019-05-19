#define KEY_SIZE 1024

#ifndef SECURITY_H
#define SECURITY_H

#include "linux/fs.h"
#include <string.h>


extern int encrypt_file(struct m_inode* inode);
extern int encrypt_block(struct buffer_head* buff_head);
extern int set_key(char* key);
extern int is_power_of_two(size_t num);
extern void userspace_string_cpy(char* usr_str, char* kernel_str);

#endif
