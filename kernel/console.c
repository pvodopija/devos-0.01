/*
 *	console.c
 *
 * This module implements the console io functions
 *	'void con_init(void)'
 *	'void con_write(struct tty_queue * queue)'
 * Hopefully this will be a rather complete VT102 implementation.
 *
 */

/*
 *  NOTE!!! We sometimes disable and enable interrupts for a short while
 * (to put a word in video IO), but this will work even for keyboard
 * interrupts. We know interrupts aren't enabled when getting a keyboard
 * interrupt, as we use trap-gates. Hopefully all is well.
 */

#include <linux/sched.h>
#include <linux/tty.h>
#include <sys/stat.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/tty.h>
#include <linux/security.h>
#include <string.h>

#define SCREEN_START 0xb8000
#define SCREEN_END   0xc0000
#define LINES 25
#define COLUMNS 80
#define NPAR 16

// My defs ...
#define TOOL_WIDTH 22
#define TOOL_HEIGHT 12
#define CHECK_BIT(var,pos) ((var) & (1<<(pos)))
// ...

extern void keyboard_interrupt(void);

static unsigned long origin=SCREEN_START;
static unsigned long scr_end=SCREEN_START+LINES*COLUMNS*2;
static unsigned long pos;
static unsigned long x,y;
static unsigned long top=0,bottom=LINES;
static unsigned long lines=LINES,columns=COLUMNS;
static unsigned long state=0;
static unsigned long npar,par[NPAR];
static unsigned long ques=0;
static unsigned char attr=0x07;


// My vars ...
short ls_files[10][16];
char clipboard[10][20];
short cliboard_cursor[2]; 		// row and column of cursor
short children_inodes[10];
short selected_row_index = 0;
short current_dir_len =0;
char init_flag = 0;
char current_dir_path[22];  	// Set to "/" in con_init();
struct m_inode* root_node;
int scrupt_state_flag = 0;
char is_dir[10];

// ...

/*
 * this is what the terminal answers to a ESC-Z or csi0c
 * query (= vt100 response).
 */
#define RESPONSE "\033[?1;2c"

// My functions ...

void render_border(char* title){
	short* tool_vmpos = origin + 2*(COLUMNS - TOOL_WIDTH);
	short* iter_vmpos = tool_vmpos;
	char path[25];
	strcpy(path, " [ ");
	strcat(path, title);
	strcat(path, " ] ");
	int path_len = strlen(path);
	char first_row[TOOL_WIDTH];
	int k, p;
	short color = 0x0300;
	if(!strcmp("CLIPBOARD", title))
		color = 0x0100;

	// Create first row with path b
	for(k=0, p=0; k<TOOL_WIDTH; k++){
		if(k >= (TOOL_WIDTH-path_len)/2 && k < (TOOL_WIDTH-path_len)/2 + path_len)
			first_row[k] = path[p++];
		else
			first_row[k] = '#';
	}

	// Render first row
	for(k=0; k<TOOL_WIDTH; k++, iter_vmpos++){
		*iter_vmpos = color | ((const char) first_row[k]);
	}
	
	// Render body
	short* i, *j;
	for(i=tool_vmpos+COLUMNS; i<tool_vmpos + (COLUMNS * TOOL_HEIGHT); i+=COLUMNS){
		*i = color | '#';
		for(j=i+1; j<i+TOOL_WIDTH-1; j++){
			*j = ((short) 0x0 << 8) | ' ';
		}
		*j = color | '#';
	}
	
	//render last row
	for(j=i; j<i+TOOL_WIDTH; j++){
		*j = color | '#';
	}
}

// DONT TOUCH THIS!!
void get_children_from_fs(){
	struct m_inode *dir_node;
	root_node = iget(0x301, 1);
	struct buffer_head* bf_head;

	if(!init_flag){
		current->pwd = root_node;
		current->root = root_node;
		bf_head = bread(0x301, root_node->i_zone[0]);

	}else{
		current->pwd = root_node;
		current->root = root_node;
		dir_node = namei(current_dir_path); 
		bf_head = bread(0x301, dir_node->i_zone[0]);
		iput(dir_node);
	}
	struct dir_entry* current_file = (struct dir_entry*) bf_head->b_data;
	

	int i,k;
	short* my_vm = origin;
	current_file += 2;

	for(k=0; k<10; k++)
		ls_files[k][0] = (short) '\0';

	current_dir_len = 0;
	for(k = 0; k<10; k++){
		char* file_name = current_file->name;
		children_inodes[k] = current_file->inode;
		char temp_name[22];
		strcpy(temp_name, current_dir_path);
		if(strcmp(current_dir_path, "/"))
			strcat(temp_name, "/");
		
		strcat(temp_name, file_name);
		struct m_inode* curr_inode = namei(temp_name);

		short color = 0x0700;
		is_dir[k] = 0;
		if(S_ISDIR((curr_inode->i_mode))){
			is_dir[k] = 1;
			color = 0x0100;
		}else if(CHECK_BIT(curr_inode->i_mode, 6)){  // Is executable by owner? (Bit 6)
			color = 0x0200;
		}
		if(CHECK_BIT(curr_inode->i_mode, 13)){       // Check if char device (Bit 13)
			color = 0x0600;
		}
		iput(curr_inode);
			

		i = 0;
		while(file_name[i] != '\0'){
			ls_files[k][i] = (color) | file_name[i++];
		}
		if(i > 0)
			current_dir_len++;

		ls_files[k][i] = (short) 0x0;

		current_file++;
	}

	
	if(!init_flag){
		init_flag = 1;	
	}

	iput(root_node);
	current->root = NULL;
	current->pwd = NULL;

}

void render_ls(){
	int i;
	for(i=0; i<10; i++){
		print_entry(ls_files[i], i);
	}

}
// called first time in con_init()
void clear_cliboard(){			
	int i;
	for(i=0; i<10; i++){
		strcpy(clipboard[i], "");
	}
}

short* get_currsor_vm_pos(){
	short* clb_vm = origin + COLUMNS*2*(cliboard_cursor[0]+2) - TOOL_WIDTH*2 + cliboard_cursor[1]*2 + 2;
	return clb_vm;
}
void print_to_clb(char c){
	if(c == 127){			// 127 -> BACKSPACE
		int len = strlen(clipboard[selected_row_index]);
		if(len == 0)
			return;
		clipboard[selected_row_index][len-1] = '\0';
		move_cursor_col(-1);
	}else{
		if(cliboard_cursor[1] + 1 > 19)
		return;
		char current_char[2];
		current_char[0] = c;
		current_char[1] = '\0';
		strcat(clipboard[selected_row_index], current_char);
		move_cursor_col(1);
	}
	print_clipboard();

}
void move_cursor_col(short pos){
	short* cursor_vm = get_currsor_vm_pos();
	*cursor_vm = 0x0000 | (char) *cursor_vm;
	cliboard_cursor[1] += pos;
	cursor_vm += pos;
	if(pos > 0)
		*cursor_vm = 0x7000 | (char) *cursor_vm;
	else if(pos < 0)
		*cursor_vm = 0x7000 | ' ';

}

void f2_handle(int f2_flag){
	scrupt_state_flag = f2_flag;
	selected_row_index = 0;
	if(f2_flag == 1){
		// creates file explorer
		render_border(current_dir_path);
		get_children_from_fs();
		render_ls();
		
	}else if(f2_flag == 2){
		// create clipbord
		cliboard_cursor[0] = 0;
		cliboard_cursor[1] = strlen(clipboard[0]);
		render_border("CLIPBOARD");
		print_clipboard();
	}

	
}

int short_len(short* str){
	int i=0;
	short temp = str[i];
	while(((char) temp) != '\0'){
		temp = str[i++];
	}
	return i-1;
}

int short_to_str(short* str, char* str1){
	short temp;
	int i = 0;
	int len = short_len(str);
	for(i=0; i<len; i++){
		temp = str[i];
		str1[i] = (char) temp;
	}

	str1[i] = '\0';
	return len;
}

void print_entry(short* entry_name, int index){
	short* ls_vm = origin + COLUMNS*2*(index+2) - TOOL_WIDTH*2 + 2;
	int name_len = short_len(entry_name);

	int i, p;
	for(i=0, p=0; i<TOOL_WIDTH-2; i++){
		if(i >= (TOOL_WIDTH-name_len)/2 && i < (TOOL_WIDTH + name_len)/2)
			*(ls_vm++) = entry_name[p++];
		else
			ls_vm++;  // moze da se stavi i ' ' zbog overwrite-a
	}

}

void print_clipboard(){
	short* clb_vm = origin + COLUMNS*4 - TOOL_WIDTH*2 + 2;

	int i;
	for(i =0; i<10; i++){
		char* line = clipboard[i];
		int j = 0;
		short* tmp_vm = clb_vm;
		while(line[j] != '\0'){
			(*tmp_vm++) = 0x0700 | line[j++]; 
		}
		clb_vm += COLUMNS;
	}
	
}

void arrow_down_handle(int f2_flag){
	if(f2_flag == 1){
		// deleting previous arrow
		short* selected_vm = origin + COLUMNS*2*(selected_row_index+2) - TOOL_WIDTH*2 + 2;
		*(selected_vm) = ((short) 0x0 << 8) | ' ';
		*(selected_vm+TOOL_WIDTH-3) = ((short) 0x0 << 8) | ' ';

		selected_row_index = (selected_row_index+1)%current_dir_len;

		// Printing new arrow
		selected_vm = origin + COLUMNS*2*(selected_row_index+2) - TOOL_WIDTH*2 + 2;
		*(selected_vm) = ((short) 0x2 << 8) | '>';
		*(selected_vm+TOOL_WIDTH-3) = ((short) 0x2 << 8) | '<';

	}else if(f2_flag == 2){

		short* clb_vm = get_currsor_vm_pos();
		*clb_vm = 0x0000 | (char) *clb_vm;

		selected_row_index = (selected_row_index+1)%10;

		cliboard_cursor[0] = selected_row_index;
		cliboard_cursor[1] = strlen(clipboard[selected_row_index]);

		clb_vm = get_currsor_vm_pos();
		*clb_vm = 0x7000 | (char) *clb_vm;
	}
}

void arrow_up_handle(int f2_flag){
	if(f2_flag == 1){
		// deleting previous arrow
		short* selected_vm = origin + COLUMNS*2*(selected_row_index+2) - TOOL_WIDTH*2 + 2;
		*(selected_vm) = ((short) 0x0 << 8) | ' ';
		*(selected_vm+TOOL_WIDTH-3) = ((short) 0x0 << 8) | ' ';

		selected_row_index--;

		if(selected_row_index < 0){
			selected_row_index = current_dir_len - 1;
		}
		// Printing new arrow
		selected_vm = origin + COLUMNS*2*(selected_row_index+2) - TOOL_WIDTH*2 + 2;
		*(selected_vm) = ((short) 0x2 << 8) | '>';
		*(selected_vm+TOOL_WIDTH-3) = ((short) 0x2 << 8) | '<';

	}else if(f2_flag == 2){

		short* clb_vm = get_currsor_vm_pos();
		*clb_vm = 0x0000 | (char) *clb_vm;

		selected_row_index--;
		if(selected_row_index < 0)
			selected_row_index = 9;

		cliboard_cursor[0] = selected_row_index;
		cliboard_cursor[1] = strlen(clipboard[selected_row_index]);
		clb_vm = get_currsor_vm_pos();
		*clb_vm = 0x7000 | (char) *clb_vm;
	}

}

void arrow_right_handle(int f2_flag){
	if(f2_flag == 1){
		if(is_dir[selected_row_index] == 0)    // Check if its NOT a directory
			return;
		char new_path[15];
		if(strcmp(current_dir_path, "/"))
			strcat(current_dir_path, "/");
		short_to_str(ls_files[selected_row_index], new_path);
		strcat(current_dir_path, new_path);

		get_children_from_fs();
		render_border(current_dir_path);
		render_ls();
	}else if(f2_flag == 2){

	}

}

void arrow_left_handle(int f2_flag){
	if(f2_flag == 1){
		if(!strcmp(current_dir_path, "/"))
			return;
		int i;
		for(i = strlen(current_dir_path); current_dir_path[i-1] != '/'; i--)
			;
		if(strcmp(current_dir_path, "/"))
			current_dir_path[i] = '\0';
		else
			current_dir_path[i+1] = '\0';

		get_children_from_fs();
		render_border(current_dir_path);
		render_ls();
	} else if(f2_flag == 2){
		
	}

}

void space_handle(int f2_flag){
	char file_to_cpy[22];
	if(f2_flag == 1){
		strcpy(file_to_cpy, current_dir_path);
		if(strcmp(current_dir_path, "/"))
			strcat(file_to_cpy, "/");

		strcat(file_to_cpy, ls_files[selected_row_index]);
		
	}else if(f2_flag == 2){
		strcpy(file_to_cpy, clipboard[selected_row_index]);
	}
	int i;
	int len = strlen(file_to_cpy);
	for(i=0; i<len; i++){
		char c = file_to_cpy[i];
		if(c>31 && c<127){
			PUTCH(c, tty_table[0].read_q);
			do_tty_interrupt(0);
		}
	}
		
}



 // ...



static inline void gotoxy(unsigned int new_x,unsigned int new_y)
{
	if (new_x>=columns || new_y>=lines)
		return;
	x=new_x;
	y=new_y;
	pos=origin+((y*columns+x)<<1);
}

static inline void set_origin(void)
{
	cli();
	outb_p(12,0x3d4);
	outb_p(0xff&((origin-SCREEN_START)>>9),0x3d5);
	outb_p(13,0x3d4);
	outb_p(0xff&((origin-SCREEN_START)>>1),0x3d5);
	sti();
}

static void scrup(void)
{
	if (!top && bottom==lines) {
		origin += columns<<1;
		pos += columns<<1;
		scr_end += columns<<1;
		if (scr_end>SCREEN_END) {
			
			int d0,d1,d2,d3;
			__asm__ __volatile("cld\n\t"
				"rep\n\t"
				"movsl\n\t"
				"movl %[columns],%1\n\t"
				"rep\n\t"
				"stosw"
				:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
				:"0" (0x0720),
				 "1" ((lines-1)*columns>>1),
				 "2" (SCREEN_START),
				 "3" (origin),
				 [columns] "r" (columns)
				:"memory");

			scr_end -= origin-SCREEN_START;
			pos -= origin-SCREEN_START;
			origin = SCREEN_START;
		} else {
			int d0,d1,d2;
			__asm__ __volatile("cld\n\t"
				"rep\n\t"
				"stosl"
				:"=&a" (d0), "=&c" (d1), "=&D" (d2) 
				:"0" (0x07200720),
				"1" (columns>>1),
				"2" (scr_end-(columns<<1))
				:"memory");
		}
		set_origin();
	} else {
		int d0,d1,d2,d3;
		__asm__ __volatile__("cld\n\t"
			"rep\n\t"
			"movsl\n\t"
			"movl %[columns],%%ecx\n\t"
			"rep\n\t"
			"stosw"
			:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
			:"0" (0x0720),
			"1" ((bottom-top-1)*columns>>1),
			"2" (origin+(columns<<1)*top),
			"3" (origin+(columns<<1)*(top+1)),
			[columns] "r" (columns)
			:"memory");
	}

	if(scrupt_state_flag == 1){
		// re-render file explorer
		render_border(current_dir_path);
		render_ls();
		
	}else if(scrupt_state_flag == 2){
		// re-render clipboard
		render_border("CLIPBOARD");
		print_clipboard();
	}

}

static void scrdown(void)
{
	int d0,d1,d2,d3;
	__asm__ __volatile__("std\n\t"
		"rep\n\t"
		"movsl\n\t"
		"addl $2,%%edi\n\t"	/* %edi has been decremented by 4 */
		"movl %[columns],%%ecx\n\t"
		"rep\n\t"
		"stosw"
		:"=&a" (d0), "=&c" (d1), "=&D" (d2), "=&S" (d3)
		:"0" (0x0720),
		"1" ((bottom-top-1)*columns>>1),
		"2" (origin+(columns<<1)*bottom-4),
		"3" (origin+(columns<<1)*(bottom-1)-4),
		[columns] "r" (columns)
		:"memory");
}

static void lf(void)
{
	if (y+1<bottom) {
		y++;
		pos += columns<<1;
		return;
	}
	scrup();
}

static void ri(void)
{
	if (y>top) {
		y--;
		pos -= columns<<1;
		return;
	}
	scrdown();
}

static void cr(void)
{
	pos -= x<<1;
	x=0;
}

static void del(void)
{
	if (x) {
		pos -= 2;
		x--;
		*(unsigned short *)pos = 0x0720;
	}
}

static void csi_J(int par)
{
	long count;
	long start;

	switch (par) {
		case 0:	/* erase from cursor to end of display */
			count = (scr_end-pos)>>1;
			start = pos;
			break;
		case 1:	/* erase from start to cursor */
			count = (pos-origin)>>1;
			start = origin;
			break;
		case 2: /* erase whole display */
			count = columns*lines;
			start = origin;
			break;
		default:
			return;
	}
	int d0,d1,d2;
	__asm__ __volatile__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		:"=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"0" (count),"1" (start),"2" (0x0720)
		:"memory");
}

static void csi_K(int par)
{
	long count;
	long start;

	switch (par) {
		case 0:	/* erase from cursor to end of line */
			if (x>=columns)
				return;
			count = columns-x;
			start = pos;
			break;
		case 1:	/* erase from start of line to cursor */
			start = pos - (x<<1);
			count = (x<columns)?x:columns;
			break;
		case 2: /* erase whole line */
			start = pos - (x<<1);
			count = columns;
			break;
		default:
			return;
	}
	int d0,d1,d2;
	__asm__ __volatile__("cld\n\t"
		"rep\n\t"
		"stosw\n\t"
		:"=&c" (d0), "=&D" (d1), "=&a" (d2)
		:"0" (count),"1" (start),"2" (0x0720)
		:"memory");
}

void csi_m(void)
{
	int i;

	for (i=0;i<=npar;i++)
		switch (par[i]) {
			case 0:attr=0x07;break;
			case 1:attr=0x0f;break;
			case 4:attr=0x0f;break;
			case 7:attr=0x70;break;
			case 27:attr=0x07;break;
		}
}

static inline void set_cursor(void)
{
	cli();
	outb_p(14,0x3d4);
	outb_p(0xff&((pos-SCREEN_START)>>9),0x3d5);
	outb_p(15,0x3d4);
	outb_p(0xff&((pos-SCREEN_START)>>1),0x3d5);
	sti();
}

static void respond(struct tty_struct * tty)
{
	char * p = RESPONSE;

	cli();
	while (*p) {
		PUTCH(*p,tty->read_q);
		p++;
	}
	sti();
	copy_to_cooked(tty);
}

static void insert_char(void)
{
	int i=x;
	unsigned short tmp,old=0x0720;
	unsigned short * p = (unsigned short *) pos;

	while (i++<columns) {
		tmp=*p;
		*p=old;
		old=tmp;
		p++;
	}
}

static void insert_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom=lines;
	scrdown();
	top=oldtop;
	bottom=oldbottom;
}

static void delete_char(void)
{
	int i;
	unsigned short * p = (unsigned short *) pos;

	if (x>=columns)
		return;
	i = x;
	while (++i < columns) {
		*p = *(p+1);
		p++;
	}
	*p=0x0720;
}

static void delete_line(void)
{
	int oldtop,oldbottom;

	oldtop=top;
	oldbottom=bottom;
	top=y;
	bottom=lines;
	scrup();
	top=oldtop;
	bottom=oldbottom;
}

static void csi_at(int nr)
{
	if (nr>columns)
		nr=columns;
	else if (!nr)
		nr=1;
	while (nr--)
		insert_char();
}

static void csi_L(int nr)
{
	if (nr>lines)
		nr=lines;
	else if (!nr)
		nr=1;
	while (nr--)
		insert_line();
}

static void csi_P(int nr)
{
	if (nr>columns)
		nr=columns;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_char();
}

static void csi_M(int nr)
{
	if (nr>lines)
		nr=lines;
	else if (!nr)
		nr=1;
	while (nr--)
		delete_line();
}

static int saved_x=0;
static int saved_y=0;

static void save_cur(void)
{
	saved_x=x;
	saved_y=y;
}

static void restore_cur(void)
{
	x=saved_x;
	y=saved_y;
	pos=origin+((y*columns+x)<<1);
}

void con_write(struct tty_struct * tty)
{
	int nr;
	char c;

	nr = CHARS(tty->write_q);
	while (nr--) {
		GETCH(tty->write_q,c);
		switch(state) {
			case 0:
				if (c>31 && c<127 && IS_VISIBLE(tty->settings)) {	/* check if visible flag is on */
					if (x>=columns) {
						x -= columns;
						pos -= columns<<1;
						lf();
					}
					__asm__("movb attr,%%ah\n\t"
						"movw %%ax,%1\n\t"
						::"a" (c),"m" (*(short *)pos)
						/*:"ax"*/);
					pos += 2;
					x++;
				} else if (c==27)
					state=1;
				else if (c==10 || c==11 || c==12)
					lf();
				else if (c==13)
					cr();
				else if (c==ERASE_CHAR(tty))
					del();
				else if (c==8) {
					if (x) {
						x--;
						pos -= 2;
					}
				} else if (c==9) {
					c=8-(x&7);
					x += c;
					pos += c<<1;
					if (x>columns) {
						x -= columns;
						pos -= columns<<1;
						lf();
					}
					c=9;
				}
				break;
			case 1:
				state=0;
				if (c=='[')
					state=2;
				else if (c=='E')
					gotoxy(0,y+1);
				else if (c=='M')
					ri();
				else if (c=='D')
					lf();
				else if (c=='Z')
					respond(tty);
				else if (x=='7')
					save_cur();
				else if (x=='8')
					restore_cur();
				break;
			case 2:
				for(npar=0;npar<NPAR;npar++)
					par[npar]=0;
				npar=0;
				state=3;
				if ((ques=(c=='?')))
					break;
			case 3:
				if (c==';' && npar<NPAR-1) {
					npar++;
					break;
				} else if (c>='0' && c<='9') {
					par[npar]=10*par[npar]+c-'0';
					break;
				} else state=4;
			case 4:
				state=0;
				switch(c) {
					case 'G': case '`':
						if (par[0]) par[0]--;
						gotoxy(par[0],y);
						break;
					case 'A':
						if (!par[0]) par[0]++;
						gotoxy(x,y-par[0]);
						break;
					case 'B': case 'e':
						if (!par[0]) par[0]++;
						gotoxy(x,y+par[0]);
						break;
					case 'C': case 'a':
						if (!par[0]) par[0]++;
						gotoxy(x+par[0],y);
						break;
					case 'D':
						if (!par[0]) par[0]++;
						gotoxy(x-par[0],y);
						break;
					case 'E':
						if (!par[0]) par[0]++;
						gotoxy(0,y+par[0]);
						break;
					case 'F':
						if (!par[0]) par[0]++;
						gotoxy(0,y-par[0]);
						break;
					case 'd':
						if (par[0]) par[0]--;
						gotoxy(x,par[0]);
						break;
					case 'H': case 'f':
						if (par[0]) par[0]--;
						if (par[1]) par[1]--;
						gotoxy(par[1],par[0]);
						break;
					case 'J':
						csi_J(par[0]);
						break;
					case 'K':
						csi_K(par[0]);
						break;
					case 'L':
						csi_L(par[0]);
						break;
					case 'M':
						csi_M(par[0]);
						break;
					case 'P':
						csi_P(par[0]);
						break;
					case '@':
						csi_at(par[0]);
						break;
					case 'm':
						csi_m();
						break;
					case 'r':
						if (par[0]) par[0]--;
						if (!par[1]) par[1]=lines;
						if (par[0] < par[1] &&
						    par[1] <= lines) {
							top=par[0];
							bottom=par[1];
						}
						break;
					case 's':
						save_cur();
						break;
					case 'u':
						restore_cur();
						break;
				}
		}
	}
	set_cursor();
}

/*
 *  void con_init(void);
 *
 * This routine initalizes console interrupts, and does nothing
 * else. If you want the screen to clear, call tty_write with
 * the appropriate escape-sequece.
 */
void con_init(void)
{
	strcpy(current_dir_path, "/");
	clear_cliboard();

	register unsigned char a;

	gotoxy(*(unsigned char *)(0x90000+510),*(unsigned char *)(0x90000+511));
	set_trap_gate(0x21,&keyboard_interrupt);
	outb_p(inb_p(0x21)&0xfd,0x21);
	a=inb_p(0x61);
	outb_p(a|0x80,0x61);
	outb(a,0x61);
}
