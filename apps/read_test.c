#include <unistd.h>
#include <string.h>

char keys[] = "abcdefg";
char buff[20];
char found_flag = 'N';

int main(int argc, char** argv){
	

	__asm__ __volatile__ (
		"leal keys, %%esi;"
		"movl $0x6, %%ecx;"
		"movb $0x63, %%al;"
		"movl $0, %%edx;"
		"leal buff, %%edi;"
		"movb (%%esi), %%bl;"
		"label: cmpb %%bl, %%al;"
			"je found;"
			"incl %%edx;"
			"leal (%%esi,%%edx,1), %%esi;"
			"leal (%%edi,%%edx,1), %%edi;"
			"movb (%%esi), %%bl;"
			"movb %%bl, (%%edi);"
			"loop label;"
			"jmp exit;"
		"found: movb $0x46, found_flag;"
		"exit: "
	:
	:
	:"memory", "di", "si", "cx", "dx", "bx", "ax"
	);

	write(1, &found_flag, 1);
	write(1, (const char*) buff, 6);

	
	_exit(0);
}
