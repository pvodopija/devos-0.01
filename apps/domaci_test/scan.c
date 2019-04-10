
/*
	Deklaracije za nizove i tabele ovde
	tabelu za prevodjenje scancodeova i tabelu
	za cuvanje mnemonika cuvati kao staticke
	varijable unutar ovog fajla i ucitavati
	ih uz pomoc funkcije load_config

*/
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "utils.h"
#define SCAN_H_INCLUDED
#include "scan.h"


char codes[BUFF_SIZE];
char sp_codes[BUFF_SIZE];
char keys[MNE_SIZE];
char* values[MNE_SIZE];

int SC_LENGTH;
int buffer_index = 0;
char shift_flag = 0;		// 0 -> not pressed, 1 -> pressed
char alt_flag = 0;
char ctrl_flag = 0;
int MNE_LENGTH;


void print_string(char* str){
	int i;
	for(i=0; str[i] != '\0'; i++)
		write(1, &str[i], 1);

}

void load_config(const char *scancodes_filename, const char *mnemonic_filename){
	int codes_fd = open(scancodes_filename, O_RDONLY);
	int mnemonics_fd = open(mnemonic_filename, O_RDONLY);

	if(codes_fd < 0 || mnemonics_fd < 0){
		printerr("Error opening files.\n");
		_exit(1);
	}
	// Loading normal codes
	SC_LENGTH = fgets(codes, BUFF_SIZE, codes_fd);
	
	// Loading special codes (SHIFT + scancode)
	fgets(sp_codes, BUFF_SIZE, codes_fd);

	// Loading mnemonics (CTRL + scancode)
	char temp_buff[MNE_VALUE_SIZE+2];
	fgets(temp_buff, MNE_VALUE_SIZE+2, mnemonics_fd);
	MNE_LENGTH = atoi(temp_buff);
	int i;
	for(i=0; i<MNE_LENGTH; i++){
		fgets(temp_buff, MNE_VALUE_SIZE+2, mnemonics_fd);
		keys[i] = temp_buff[0];
		strcpy(values[i], temp_buff+2);  // temp_buff[0] -> key, tmp_buff[1] = ' ', temp_buff[2] -> value	
		
	}
}
int process_scancode(int scancode, char *buffer){

	__asm__ __volatile__ (
		"pushl %%edi;"
		"cmpl %%ebx, %%eax;"
		"jge special_code;"
		"cmpb $0x1, shift_flag;"
		"je shift_chars;"
		"leal (%%edx,%%eax,1), %%esi;"
		"jmp move_string;"
		"shift_chars: leal (sp_codes), %%edx;"
			"leal (%%edx,%%eax,1), %%esi;"
		"move_string:" 
			"movl buffer_index, %%ebx;"
			"leal (%%edi,%%ebx,1), %%edi;"
			"movsb;"
			"cmpb $0x1, alt_flag;"
			"je inc_buff_i;"
			"cmpb $0x1, ctrl_flag;"
			"je write_mnemonic;"
			"jmp exit;"

		"special_code:"
			"cmpl " XSTR(SHIFT_DOWN)", %%eax;"
			"je shift_handler;"
			"cmpl " XSTR(SHIFT_UP) ", %%eax;" 
			"je shift_handler;"
			"cmpl " XSTR(ALT_DOWN) ", %%eax;"
			"je alt_handler;"
			"cmpl " XSTR(ALT_UP) ", %%eax;"
			"je alt_up_handler;"
			"cmpl " XSTR(CTRL_DOWN) ", %%eax;"
			"je ctrl_handler;"
			"cmpl " XSTR(CTRL_UP) ", %%eax;"
			"je ctrl_up_handler;"
			"jmp exit;"
			
			"shift_handler: movb shift_flag, %%bl;"
				"xorb $0x1, %%bl;" // flips only the first bit
				"movb %%bl, shift_flag;"
				"jmp exit;"		
			"inc_buff_i: incl buffer_index;"
				"jmp exit;"
			"alt_up_handler:"
				"movl buffer_index, %%ebx;"
				"incl %%ebx;"
				"movb $0x0, (%%edi,%%ebx,1);" // 0x0 = '\0'
				"movl $0x0, buffer_index;"
			"alt_handler: movb alt_flag, %%bl;"
				"xorb $0x1, %%bl;" // flips only the first bit
				"movb %%bl, alt_flag;"
				"jmp exit;"
			"ctrl_up_handler: movl $0x0, buffer_index;"
			"ctrl_handler: movb ctrl_flag, %%bl;"
				"xorb $0x1, %%bl;"
				"movb %%bl, ctrl_flag;"
				"jmp exit;"
			"write_mnemonic: pop %%edi;"
				"push %%edi;"
				"xorl %%ecx, %%ecx;"
				"xorl %%edx, %%edx;"
				"xorl %%ebx, %%ebx;"
				"leal keys, %%esi;"
				"movb (%%edi), %%dl;"	
				"movb MNE_LENGTH, %%cl;"
				"search: cmpb (%%esi), %%dl;"
					"je found;"
					"incl %%esi;"
					"incl %%ebx;"
					"loop search;"
					"jmp exit;"
				"found: "
					"movl $0x0, buffer_index;"
					"leal values, %%esi;"
					"movl (%%esi,%%ebx,4), %%esi;"
					"str_cpy: movsb;"
						"incl buffer_index;"
						"cmpb $0x0, (%%esi);"
						"jne str_cpy;"
						"jmp exit;"
					"jmp exit;"


				"jmp exit;"
	"exit: popl %%edi;"
		:
		:"D" (buffer), "a" (scancode), "b" (SC_LENGTH),"d" (codes)
		:"memory", "cx"
	);

	if(scancode == SHIFT_DOWN || scancode == SHIFT_UP || scancode == CTRL_DOWN || scancode == CTRL_UP)
		return 0;
	else if(scancode != ALT_UP)
		buffer[buffer_index+1] = '\0';
	
	return buffer_index+1;
}
