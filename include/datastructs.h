#define STACK_SIZE 100
#define TABLE_SIZE 61   // 61 is prime and closest to 64
#define HKEY_SIZE 14
#ifndef DATASTRUCTS_H
#define DATASTRUCTS_H

struct e_dir{
    unsigned short i_num;
    char h_key[HKEY_SIZE];   // hashed key value
};

typedef struct Stack{
    struct m_inode* values[STACK_SIZE];
    struct m_inode** sp;

    void (*push)(struct Stack* self, struct m_inode* value);
    struct m_inode* (*pop)(struct Stack* self);
    char (*is_empty)(struct Stack* self);
}Stack;

// Stack functions
extern void push_func(Stack* self, struct m_inode* value);

extern struct m_inode* pop_func(Stack* self);

extern char is_empty_func(Stack* self);

typedef struct HashTable{
    struct e_dir values[TABLE_SIZE];

    int (*add)(struct HashTable* self, struct e_dir* value);
    int (*remove)(struct HashTable* self, unsigned short inode_num);
    int (*search)(struct HashTable* self, unsigned short inode_num);
}HashTable;

// Table functions
extern int add_func(struct HashTable* self, struct e_dir* value);
extern int remove_func(struct HashTable* self, unsigned short inode_num);
extern int search_func(struct HashTable* self, unsigned short inode_num);
extern void print_func(struct HashTable* self);

// encrypted dir structure


#endif