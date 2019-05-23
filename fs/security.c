#include <string.h>
#include <linux/fs.h>
#define SECURITY_INCLUDED
#include <linux/security.h>
#include <asm/segment.h>

char encryption_key[KEY_SIZE];

int encrypt_file(struct m_inode* inode){

    struct buffer_head* buff_head = bread(0x301, inode->i_zone[0]);
    if(!buff_head){
        printk("Unable to read block\n");
    }else{
        encrypt_block(buff_head, buff_head->b_data);
    }
    buff_head->b_dirt = 1;  // set dirty flag for sync()
    brelse(buff_head);
    
    printk("\ndone.\n");
    
    return 0;
}

int decrypt_file(struct m_inode* inode){
   
    struct buffer_head* buff_head = bread(0x301, inode->i_zone[0]);
    if(!buff_head){
        printk("Unable to read block\n");
    }else{
        decrypt_block(buff_head, buff_head->b_data);
    }
    buff_head->b_dirt = 1;  // set dirty flag for sync()
    brelse(buff_head);
    
    printk("\ndone.\n");
    


    return 0;
}



int set_key(char* key){
    if(is_power_of_two(strlen(key))){
        strcpy(encryption_key, key);
        return 0;
    }

    printk("error: key must be the power of 2\n");

    return 1;

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
    char key[KEY_SIZE], value[BLOCK_SIZE], encrypted[BLOCK_SIZE];
    char* columns[KEY_SIZE];
    char* offset_ptr;
    int i, j, k, key_len, col_size, curr_min;

    strcpy(key, encryption_key);

    for(i=0; i<BLOCK_SIZE; i++)
        value[i] = buff_head->b_data[i];
  
    key_len = strlen(key); 

    col_size = BLOCK_SIZE / key_len;
    offset_ptr = encrypted;
    
    // building matrix
    for(i=0, k=0; i<BLOCK_SIZE; i++, k = (k+col_size)%(BLOCK_SIZE-1)){
        encrypted[k] = value[i];
    }
    encrypted[0] = value[0];

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
            value[k++] = *(columns[j]+i);
    }

    for(i=0; i<BLOCK_SIZE; i++)
            buffer[i] = value[i];
    
    // printing encrypted data
    for(i=0; i<BLOCK_SIZE ;i++){
        printk("%c", value[i]);
    }

    return 0;
}

int decrypt_block(struct buffer_head* buff_head, char* buffer){
    char sorted_key[KEY_SIZE];
    char* columns[KEY_SIZE], *offset_ptr;
    char data[BLOCK_SIZE];
    char decrypted[BLOCK_SIZE];
    int i, j, k;

    strcpy(sorted_key, encryption_key);

    // copying to temp data buffer
    for(i=0; i<BLOCK_SIZE; i++){
        data[i] = buff_head->b_data[i];
    }

    int key_len = strlen(sorted_key);
    int col_size = BLOCK_SIZE/key_len;
    offset_ptr = decrypted;
    
    // building matrix
    for(i=0, k=0; i<BLOCK_SIZE; i++, k = (k+col_size)%(BLOCK_SIZE-1)){
        decrypted[k] = data[i];
    }
    decrypted[0] = data[0];

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
            if(encryption_key[i] == sorted_key[j]){
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
            data[k++] = *(columns[j]+i);
    }


    // printing decrypted data
    for(i=0; i<BLOCK_SIZE; i++){
        printk("%c", data[i]);
    }
    
    // TODO: change to buffer[k++] = *(columns[j]+i)
    for(i=0; i<BLOCK_SIZE; i++)
            buffer[i] = data[i];
    
    return 0;
}