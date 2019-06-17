#define KEY_SIZE 32
#define CHARSET_SIZE 65
#define ENC_TABLE_INODE 126	// inode number of reserved list of encrypyed files

#define ERR_ALR_ENC -1  // file already encrypred
#define SUCC_ADD_ENC 1  // file added succesfully
#define ERR_NOT_FND -1  // file not found
#define SUCC_FND 1    	// file found
#define ERR_RM -1		// file not removed
#define SUCC_RM 1		// file successfully removed

#define K_GLOBAL 2
#define K_LOCAL 1
#define LOCK_TIMEOUT 45		// local key timeout
#define GLOK_TIMEOUT 120	// global key timeout

#define HASH1(x) x%TABLE_SIZE
#define HASH2(x) 1+(x%(TABLE_SIZE-1))


#define clear_block(addr) \
__asm__("cld\n\t" \
	"rep\n\t" \
	"stosl" \
	::"a" (0),"c" (BLOCK_SIZE/4),"D" ((long) (addr))/*:"cx","di"*/);

#define string_copy(dest,source,n)\
__asm__(\
	"cld;"\
	"rep;"\
	"movsb;"\
	::"c" (n),"S" ((long) (source)),"D" ((long) (dest)));


#ifndef SECURITY_H
#define SECURITY_H

#include <errno.h>
#include <datastructs.h>

char encryption_key[KEY_SIZE];
char h_key[HKEY_SIZE];
char* key_to_use;
char* hash_to_use;

HashTable enc_table;

/* ENCRYPTION */
extern int encrypt_block(struct buffer_head* buff_head, char* buffer);
extern int encrypt_file(struct m_inode* inode, struct e_dir* new_dir);
extern int encrypt_directory(struct m_inode* inode);

/* DECRYPTION */
extern int decrypt_block(struct buffer_head* buff_head, char* buffer);
extern int decrypt_file(struct m_inode* inode);
extern int decrypt_directory(struct m_inode* inode);

/* KEY MANAGER */
extern int set_key(char* key, int glob_flag);
extern int rnd_key_gen(char* key, int level);
extern int clear_key();
extern int is_power_of_two(size_t num);
extern void userspace_string_cpy(char* kernel_str, char* usr_str);
extern void load_hash_table();
extern int compare_hash(char* hash1, char* hash2);

#endif
