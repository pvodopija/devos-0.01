make clean
make 2>&1 | grep --color -iP "\^|warning:|error|:"
qemu-system-i386 -hdb hd_oldlinux.img -fda Image -boot a
