#define BUFF_SIZE 128
#define MNE_SIZE 16
#define MNE_VALUE_SIZE 64
#define END_OF_FILE 400
#define SHIFT_DOWN 200
#define SHIFT_UP 300
#define CTRL_DOWN 201
#define CTRL_UP 301
#define ALT_DOWN 202
#define ALT_UP 302
#define STR(X) #X
#define XSTR(X) "$"STR(X)




#ifndef SCAN_H_INCLUDED
#define SCAN_H_INCLUDED

extern char alt_flag;


void load_config(const char *scancodes_filename, const char *mnemonic_filename);
int process_scancode(int scancode, char *buffer);



#endif
