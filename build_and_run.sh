#! /bin/bash
if ./compile.sh $1; then
mkdir tmp_hd
sudo mount -o loop,offset=10240 hd_oldlinux.img tmp_hd
sudo cp ./apps/$1.bin ./tmp_hd/root/$1.bin
sleep .5
sudo umount tmp_hd
rmdir tmp_hd
read -p "Build successful. Would you like to run it?[Y/n]" -n 1 -r
if [[ $REPLY =~ ^[Yy]$ || ${#REPLY} -eq 0 ]]; then
./start.sh
fi
else
echo 'Build failed.'
fi
echo
