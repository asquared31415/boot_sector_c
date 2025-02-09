#!/usr/bin/env fish

# workaround to spawn qemu only once socat is set up
fish -c 'while not test -e qemu_pty; sleep 0.1; end; qemu-system-i386 -drive format=raw,file=qemu_img -chardev serial,id=char0,path=qemu_pty -serial chardev:char0 -display none -D out/qemu.log -s' &
# create file linked to pty for qemu to connect to
socat - pty,link=qemu_pty
# clear output when socat exits
clear
