#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFF_SIZE 128

int main(void){
	int fd = open("scancodes.tbl", 0_RDONLY);
	if(fd < 0)
		print("ERROR\n");
	char buff[BUFF_SIZE];
	int len = read(fd, buff, BUFF);

	write(1, (const char*) buff, BUFF_SIZE);

	_exit(0);

}
