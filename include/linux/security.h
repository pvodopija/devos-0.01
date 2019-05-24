#define KEY_SIZE 1024
#define ENC_LIST_INODE 134

#define ERR_ALR_ENC -1  // file already encrypred
#define SUCC_ADD_ENC 1  // file added succesfully
#define ERR_NOT_FND -1  // file not found
#define SUCC_FND 1    // file found


#ifndef SECURITY_H
#define SECURITY_H


#include <linux/fs.h>
#include <string.h>


#define clear_block(addr) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"stosl" \
	::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr))/*:"cx","di"*/)

extern int encrypt_file(struct m_inode* inode);
extern int encrypt_block(struct buffer_head* buff_head, char* buffer);
extern int decrypt_file(struct m_inode* inode);
extern int decrypt_block(struct buffer_head* buff_head, char* buffer);
extern int set_key(char* key);
extern int is_power_of_two(size_t num);
extern void userspace_string_cpy(char* kernel_str, char* usr_str);
extern int add_enc_list(struct dir_entry* new_dir);
extern int clear_enc_list();
extern int print_enc_list();
extern int in_enc_list(short inode_num);

#endif
