#define KEY_SIZE 1024
#define CHARSET_SIZE 128
#define ENC_LIST_INODE 134

#define ERR_ALR_ENC -1  // file already encrypred
#define SUCC_ADD_ENC 1  // file added succesfully
#define ERR_NOT_FND -1  // file not found
#define SUCC_FND 1    	// file found
#define SUCC_RM 1		// file successfully removed
#define ERR_RM -1		// file not removed

#ifndef SECURITY_H
#define SECURITY_H

#include <linux/fs.h>
#include <string.h>

char encryption_key[KEY_SIZE];

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
extern int rnd_key_gen(char* key, int level);
extern int clear_key();
extern int is_power_of_two(size_t num);
extern void userspace_string_cpy(char* kernel_str, char* usr_str);
extern int add_enc_list(struct dir_entry* new_dir);
extern int rm_enc_list(short i_num);
extern int clear_enc_list();
extern int print_enc_list();
extern int in_enc_list(short inode_num);

#endif
