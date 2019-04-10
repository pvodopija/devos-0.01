#include <unistd.h>
#include <fcntl.h>

#include "scan.h"

#define UTIL_IMPLEMENTATION
#include "utils.h"

int main(int argc, char **argv){
	/* ucitavanje podesavanja */
	
	char sc_filename[50];
	int char_Len = read(0, sc_filename, 50);
	sc_filename[char_Len-1] = '\0';

	load_config(sc_filename, "ctrl.map");	

	/* ponavljamo: */
		/* ucitavanje i otvaranje test fajla */
		/* parsiranje fajla, obrada scancodova, ispis */
	
	int test_fd = open("test1.tst", O_RDONLY);
	int len, scancode;
	char line_buff[BUFF_SIZE];
	char buff[BUFF_SIZE];
	
	do{
		len = fgets(line_buff, BUFF_SIZE, test_fd);
		scancode = atoi(line_buff);
		len = process_scancode(scancode, buff);
		if(scancode == ALT_UP){
			char ascii_char = (char) atoi(buff);
			write(1, &ascii_char, 1);
		}else if(!alt_flag){
			write(1, buff, len);
		}
	}while(scancode != END_OF_FILE);


	_exit(0);
}
