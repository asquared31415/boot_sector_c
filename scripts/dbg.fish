#!/usr/bin/env fish
if scripts/build.fish
    # -S here causes qemu to wait for gdb
    nohup qemu-system-i386 -drive format=raw,file=qemu_img -serial unix:qemu_socket,server,nowait -D qemu.log -d int,cpu_reset -s -S &> /dev/null &

    # Give time for qemu to start and create the socket
    sleep 1
    RUST_LOG="info" cargo run --quiet --manifest-path=bin_transfer/Cargo.toml -- --socket unix-connect:qemu_socket --transfer-bin program.c --no-checksum
    # netcat -U qemu_socket | stdbuf -o 0 hexdump -v -e '/1 "%02X "' # HEX MODE

    # clear socat/hex output when done
    clear
end
