gcc -m32 -o $1 $1.c -nostdlib -nostdinc -e main -Ttext=100 -static -fno-builtin ../lib/lib.a -I ../include
