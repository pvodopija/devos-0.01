#include <unistd.h>
#include <string.h>

#define MAX_LEN 10

int strToInt(char* buff, int len);
void intToStr(int n, char* str);

int main(int argc, char** argv){
	char num1[MAX_LEN];
	char num2[MAX_LEN];

	int num1Length = read(0, num1, MAX_LEN);
	int num2Length = read(0, num2, MAX_LEN);

	int x = strToInt(num1, num1Length-1);
	int y =	strToInt(num2, num2Length-1);
	int result = x + y;
	
	char strResult[10];
	intToStr(result, strResult);
	
	write(1, (const char*) strResult, strlen(strResult));
	

	_exit(0);
}
	
int strToInt(char* buff, int len){
	int sum = 0;
	int tenPow = 1;
	int i;
	for(i=len-1; i>=0; i--){
		sum += (buff[i] - '0')*tenPow;
		tenPow *= 10;
	}
	return sum;
}

void intToStr(int n, char* str){
	int x = n;
	int size = 0;

	while(x > 0){
		x /= 10;
		size++;
	}

	str[size] = '\n';
	str[size+1] = '\0';
	int i;
	for(i=size-1; i>=0; i--){
		str[i] = n%10 + '0';
		n /= 10;
	}
}













