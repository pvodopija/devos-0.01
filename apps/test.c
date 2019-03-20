#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define BUFF_SIZE 128



int main(void){
	int fd = open("scancodes.tbl", O_RDONLY);
	if(fd < 0)
		write(1, "ERROR\n", strlen("ERROR\n"));
	char buff[BUFF_SIZE];
	int len = read(fd, buff, BUFF_SIZE);

	write(1, (const char*) buff, len);

	_exit(0);

}
