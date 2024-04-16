#!/usr/bin/env fish
if scripts/build.fish
    # -S here causes qemu to wait for gdb
    nohup qemu-system-x86_64 --trace "memory_region_ops_*" -drive format=raw,file=qemu_img -serial unix:qemu_socket,server,nowait -D qemu.log -d int,cpu_reset -s -S &> /dev/null &

    # Give time for qemu to start and create the socket
    sleep 3
    RUST_LOG="info" cargo run --quiet --manifest-path=bin_transfer/Cargo.toml -- --socket unix-connect:qemu_socket --transfer-bin program.c --no-socat --no-checksum
    # netcat -U qemu_socket | stdbuf -o 0 hexdump -v -e '/1 "%02X "' # HEX MODE
    # socat -,raw,echo=0 unix-connect:qemu_socket # TEXT MODE

    # clear socat/hex output when done
    # clear
end
