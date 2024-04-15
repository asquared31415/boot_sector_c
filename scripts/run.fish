#!/usr/bin/env fish
if scripts/build.fish
    nohup qemu-system-x86_64 -drive format=raw,file=qemu_img -serial unix:qemu_socket,server -D qemu.log -d int,cpu_reset -s &> /dev/null &

    # Give time for qemu to start and create the socket
    sleep 1
    netcat -U qemu_socket | stdbuf -o 0 hexdump -v -e '/1 "%02X "' # HEX MODE
    # socat -,raw,echo=0 unix-connect:qemu_socket # TEXT MODE

    # clear socat output when done
    clear
end
