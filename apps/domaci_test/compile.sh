gcc -m32 -o domaci.bin main.c scan.c -nostdlib -nostdinc -e main -Ttext=100 -static -fno-builtin ../../lib/lib.a -I ../../include
