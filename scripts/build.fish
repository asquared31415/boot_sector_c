#!/usr/bin/env fish
mkdir -p out
rm -f out/boot.bin
nasm -w+all -w+error -f bin boot.asm -o out/boot.bin; and \
dd if=out/boot.bin of=qemu_img bs=512 count=1 conv=notrunc status=none
