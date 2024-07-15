#!/usr/bin/env fish
losetup --partscan /dev/loop0 qemu_img
mount --mkdir --read-write /dev/loop0p1 /mnt/qemu
