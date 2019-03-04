gcc -m32 ./apps/$1.c -o ./apps/$1.bin -nostdlib -nostdinc -e main -Ttext=100 -static -fno-builtin ./lib/lib.a -I ./include
