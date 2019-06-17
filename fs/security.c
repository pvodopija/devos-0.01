#include <string.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <asm/segment.h>
#include <sys/stat.h>
#include <linux/security.h>

int encrypt_file(struct m_inode* inode, struct e_dir* new_dir){
    int status;
    if((status = enc_table.add(&enc_table, new_dir)) == ERR_ALR_ENC){
		printk("error: File is already encrypted.\n");
        return status;
    }

    struct buffer_head* buff_head;
    int block;
    int i = 0;

    // Encrypting block by block
    while((block = bmap(inode, i++)) && (buff_head = bread(inode->i_dev, block))){
        encrypt_block(buff_head, buff_head->b_data);
        buff_head->b_dirt = 1;  /* set dirty flag for sync() */
        brelse(buff_head);
    }
    
    return status;
}

int decrypt_file(struct m_inode* inode){
    int status;
    if((status = enc_table.remove(&enc_table, inode->i_num)) == -EINVAL){
			printk("error: Incorrect key.\n");
			return status;
	}else if(status == ERR_RM){
        printk("error: Decrypting file.\n");
        return status;
    }

    struct buffer_head* buff_head;
    int block;
    int i = 0;

    // Encrypting block by block
    while((block = bmap(inode, i++)) && (buff_head = bread(inode->i_dev, block))){
        decrypt_block(buff_head, buff_head->b_data);
        buff_head->b_dirt = 1;  /* set dirty flag for sync() */
        brelse(buff_head);
    }
    
    
    // printk("done.\n");


    return status;
}

int encrypt_directory(struct m_inode* inode){
    Stack stack;
    stack.pop = pop_func;
    stack.push = push_func;
    stack.is_empty = is_empty_func;
    stack.sp = stack.values;

    if(enc_table.search(&enc_table, inode->i_num) != ERR_NOT_FND){
        printk("error: Directory already encrypted.\n");
        return -EINVAL;
    }
    
    printk("Encrypting directory...\n");
    
    stack.push(&stack, inode);
    
    while(!stack.is_empty(&stack)){
        struct m_inode* current_node = stack.pop(&stack);
        if(!current_node){
            printk("nullptr exception\n");
            return -1;
        }

        struct buffer_head* buff_head = bread(current_node->i_dev, current_node->i_zone[0]);
        struct dir_entry* current_dir = (struct dir_entry*) buff_head->b_data;

        // skip . and .. dirs
        current_dir += 2;

        while(current_dir->inode > 0){
            if(current_dir->name[0] == '.'){ /* skip hidden folders and files */
                    current_dir++;
                    continue;
            }
            struct m_inode* tmp = iget(current_node->i_dev, current_dir->inode);

            if(tmp && S_ISDIR(tmp->i_mode))
                stack.push(&stack, tmp);
            else{
                printk("encrypting %s...\n", current_dir->name);
                struct e_dir new_dir;
                new_dir.i_num = current_dir->inode;
                string_copy(new_dir.h_key,hash_to_use,HKEY_SIZE);
                encrypt_file(tmp, &new_dir);
                iput(tmp);
            }

            current_dir++;
        }
        struct e_dir dir;
        dir.i_num = current_node->i_num;
        string_copy(dir.h_key,hash_to_use,HKEY_SIZE);
        encrypt_file(current_node, &dir);

        brelse(buff_head);
        if(current_node != inode)
            iput(current_node);

    }

    return 1;
}

int decrypt_directory(struct m_inode* inode){
    Stack stack;
    stack.pop = pop_func;
    stack.push = push_func;
    stack.is_empty = is_empty_func;
    stack.sp = stack.values;

    int index;

    if((index = enc_table.search(&enc_table, inode->i_num)) == ERR_NOT_FND){
        printk("error: Directory not encrypted.\n");
        return -EINVAL;
    }
    if(compare_hash(hash_to_use, enc_table.values[index].h_key) == -EINVAL){
        printk("error: Incorrect key.");
        return -EINVAL;
    }

    printk("Decrypting directory...\n");
    
    stack.push(&stack, inode);
    
    while(!stack.is_empty(&stack)){
        struct m_inode* current_node = stack.pop(&stack);
        if(!current_node){
            printk("nullptr exception\n");
            return -1;
        }

        decrypt_file(current_node);

        struct buffer_head* buff_head = bread(current_node->i_dev, current_node->i_zone[0]);
        struct dir_entry* current_dir = (struct dir_entry*) buff_head->b_data;
        // skip . and .. dirs
        current_dir += 2;

        while(current_dir->inode > 0){
            if(current_dir->name[0] == '.'){ /* skip hidden folders and files */
                    current_dir++;
                    continue;
            }

            struct m_inode* tmp = iget(current_node->i_dev, current_dir->inode);

            if(tmp && S_ISDIR(tmp->i_mode))
                stack.push(&stack, tmp);
            else{
                printk("decrypting %s...\n", current_dir->name);
                decrypt_file(tmp);
                iput(tmp);
            }
            
            
            current_dir++;
        }
        
        brelse(buff_head);
        if(current_node != inode)
            iput(current_node);

    }
    

    return 1;
}

int set_key(char* key, int glob_key){
    // print_func(&enc_table);

    /* Doesnt work without for some reason. Probably some buffer issue since \n clears it */
    printk("\n");

    if(is_power_of_two(strlen(key))){
        if(glob_key == K_LOCAL){
            strcpy(current->key, key);
            // hash value of new local key
            hash(key, current->h_key, HKEY_SIZE);
            current->k_timeout = jiffies+HZ*LOCK_TIMEOUT;   /* set timer for 45 seconds */
        }else{
            strcpy(encryption_key, key);
            // hash value of new key
            hash(encryption_key, h_key, HKEY_SIZE);
            task[0]->k_timeout = jiffies+HZ*GLOK_TIMEOUT;   /* global timer is in init task( task[0]) */
            
        }

        key_to_use = (!current->key[0])?encryption_key:current->key;
	    hash_to_use = (!current->key[0])?h_key:current->h_key;

        return 0;
    }

    printk("error: Key must be the power of 2\n");

    return 1;

}

int rnd_key_gen(char* key, int level){
    char char_set[CHARSET_SIZE] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ123456789";
    unsigned long set_len = strlen(char_set);

    level = 0x2 << level; /* length of new key 2^(level+1) */

    char* c_ptr = key;

    while(level--){
        unsigned long index;
        __asm__ __volatile__ (
            "RDTSC;"                /* get number of ticks since cpu powered on (puts result in EAX and EDX) */
	        "addl %%eax, %%edx;"
            "movl %%edx, %0;"
        :"=r" (index)
        :"a" (set_len)
        :"%edx", "memory"
        );
	
	    index %= set_len;
        *(c_ptr++) = char_set[index];
        char_set[index] = char_set[--set_len];
        char_set[set_len] = '\0';
    }
    *c_ptr = '\0';

    return 1;
}

int clear_key(int glob_flag){
    if(glob_flag == K_GLOBAL){
        h_key[0] = '\0';
        *encryption_key = '\0';
    }else{
        current->key[0] = '\0';
        current->h_key[0] = '\0';
    }
    return 0;
}


// returns 1 if power of two, otherwise 0
int is_power_of_two(size_t num){
    if(num < 2){
        return 0;
    }

    return !(num & (num-1));
}

// copying string from user space to kernel space
void userspace_string_cpy(char* kernel_str, char* usr_str){
    char c;
    while((c=get_fs_byte(usr_str++)) != '\0'){
        *(kernel_str++) = c;
    }
    *kernel_str = '\0';

}

int encrypt_block(struct buffer_head* buff_head, char* buffer){
    char key[KEY_SIZE], encrypted[BLOCK_SIZE];
    char* columns[KEY_SIZE];
    char* offset_ptr;
    int i, j, k, key_len, col_size, curr_min;


    strcpy(key, key_to_use);
  
    key_len = strlen(key); 

    col_size = BLOCK_SIZE / key_len;
    offset_ptr = encrypted;
    
    // building matrix
    for(i=0, k=0; i<BLOCK_SIZE; i++, k = (k+col_size)%(BLOCK_SIZE-1)){
        encrypted[k] = buff_head->b_data[i];
    }
    encrypted[0] = buff_head->b_data[0];

    // setting up column pointers
    for(i=0; i<key_len; i++, offset_ptr+=col_size){
        columns[i] = offset_ptr;
    }

    // sorting key and rearanging columns
    for(i=0; i<key_len; i++){
        curr_min = key[i];

        for(j=i; j<key_len; j++){
            if(key[j] < curr_min){
                char* temp = columns[i];
                columns[i] = columns[j];
                columns[j] = temp; 
                curr_min = key[j];
                char c = key[i];
                key[i] = key[j];
                key[j] = c;
            }
        }
        
    }    

    
    // copying back to original buffer
    for(i=0, k = 0; i<col_size; i++){
        for(j=0; j<key_len; j++)
            buffer[k++] = *(columns[j]+i);
    }
    

    return 0;
}

int decrypt_block(struct buffer_head* buff_head, char* buffer){
    char sorted_key[KEY_SIZE];
    char* columns[KEY_SIZE], *offset_ptr;
    char decrypted[BLOCK_SIZE];
    int i, j, k;
     
    strcpy(sorted_key, key_to_use);

    int key_len = strlen(sorted_key);
    int col_size = BLOCK_SIZE/key_len;
    offset_ptr = decrypted;
    
    // building matrix
    for(i=0, k=0; i<BLOCK_SIZE; i++, k = (k+col_size)%(BLOCK_SIZE-1)){
        decrypted[k] = buff_head->b_data[i];
    }
    decrypted[0] = buff_head->b_data[0];

    // setting up column pointers
    for(i=0; i<key_len; i++, offset_ptr+=col_size){
        columns[i] = offset_ptr;
    }
    
    // sorting current key
    for(i=0; i<key_len; i++){
        for(j=i; j<key_len; j++){
            if(sorted_key[j] < sorted_key[i]){
                char c = sorted_key[i];
                sorted_key[i] = sorted_key[j];
                sorted_key[j] = c;
            }
        }
    }

    // rearanging columns to decrypt data
    for(i=0; i<key_len; i++){
        for(j=i; j<key_len; j++){
            if(key_to_use[i] == sorted_key[j]){
                char tmp_c = sorted_key[i];
                sorted_key[i] = sorted_key[j];
                sorted_key[j] = tmp_c;
                char* tmp_ptr = columns[i];
                columns[i] = columns[j];
                columns[j] = tmp_ptr;
            }
        }
    }

    // copying back to buffer
    for(i=0, k = 0; i<col_size; i++){
        for(j=0; j<key_len; j++)
            buffer[k++] = *(columns[j]+i);
    }

    
    return 0;
}


// add new entry to hash table
int add_func(HashTable* self, struct e_dir* value){
    int i = 0;
    int index;

    if(!value)
        return ERR_RM;

    do{
        index = (HASH1(value->i_num) + (i++)*HASH2(value->i_num))%TABLE_SIZE;

        if(self->values[index].i_num == value->i_num)
            return ERR_ALR_ENC;

    }while(self->values[index].i_num);

    self->values[index] = *value;

    return index;
}

int remove_func(HashTable* self, unsigned short i_num){
    int i = 0;
    int index;
    char found = 0;

    index = (HASH1(i_num) + (i++)*HASH2(i_num))%TABLE_SIZE;
    while(self->values[index].i_num){
        
        if(self->values[index].i_num == i_num && !found){
            
            if(compare_hash(hash_to_use, self->values[index].h_key) == -EINVAL)
                return -EINVAL;
            
            found = 1;
            self->values[index].i_num = 0;
            index = (HASH1(i_num) + (i++)*HASH2(i_num))%TABLE_SIZE;
        }else if(found){
            struct e_dir tmp = self->values[index];
            self->values[index].i_num = 0;
            self->add(self, &tmp);
        }
        
        index = (HASH1(i_num) + (i++)*HASH2(i_num))%TABLE_SIZE;
    }

    if(found)
        return SUCC_RM;

    return ERR_RM;

}

// search hash table, returns index if found
int search_func(HashTable* self, unsigned short i_num){
    int i = 0;
    int index;

    do{
        index = (HASH1(i_num) + (i++)*HASH2(i_num))%TABLE_SIZE;

        if(self->values[index].i_num == i_num)
            return index;

    }while(self->values[index].i_num);

    return ERR_NOT_FND;
}

void print_func(HashTable* self){
    int i;
    for(i=0; i<TABLE_SIZE; i++)
        if(self->values[i].i_num > 0){
            printk("%hi - ", self->values[i].i_num);
            int j;
            for(j=0; j<HKEY_SIZE; j++)
                printk("%c", self->values[i].h_key[j]);
            printk("\n");
        }
}


// I know it's horrible, but didn't have time to implement a better one...
void hash(char *str, char* dest, short len){
    unsigned long hash = 5381;
    int c, i;
    char diff = 'Z' - 'A';
    int l = strlen(str);
    for(i=0; i<len; i++){
        hash = ((hash << 5) + hash) + str[i%l]; /* hash * 33 + c */
        *(dest++) = 'A' + hash%diff;
    }

}

void clear_table(HashTable* self){
    int i;
    for(i=0; i<TABLE_SIZE; i++)
        self->values[i].i_num = 0;
}

void push_func(Stack* self, struct m_inode* value){
    (*(self->sp++)) = value;
}

struct m_inode* pop_func(Stack* self){
    return (*(--self->sp));
}

 char is_empty_func(Stack* self){
    return (self->sp == self->values);
}

void load_hash_table(){
     // filling hash table from disk
	struct m_inode* enc_inode = iget(0x301, ENC_TABLE_INODE);  // Get reserved directory for list of encrypted files
    struct buffer_head* buff_head = bread(enc_inode->i_dev, enc_inode->i_zone[0]);
    struct e_dir* current_dir = (struct e_dir*) buff_head->b_data;
	enc_table.add = add_func;
	enc_table.remove = remove_func;
	enc_table.search = search_func;
    int i;
	for(i=0; i<TABLE_SIZE; i++)
		enc_table.values[i] = *(current_dir++);

    brelse(buff_head);
    iput(enc_inode);
	//
}

int compare_hash(char* hash1, char* hash2){
    int i;
    for(i=0; i<HKEY_SIZE; i++){
        if(hash1[i] != hash2[i])
            return -EINVAL;     /* key does not match with hash value */
    }

    return 1;
}