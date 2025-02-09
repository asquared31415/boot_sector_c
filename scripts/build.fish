#!/usr/bin/env fish
mkdir -p out
rm -f out/boot.bin
nasm -w+all -w+error -w-reloc -f bin boot.asm -o out/boot.bin; and \
dd if=out/boot.bin of=qemu.img bs=512 count=1 conv=notrunc status=none ; and \
dd if=test_fs.bin of=qemu.img bs=512 seek=1 conv=notrunc status=none
