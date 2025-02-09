#!/usr/bin/env fish
RUST_LOG="info" cargo run --quiet --manifest-path=bin_transfer/Cargo.toml -- --socket /dev/ttyUSB0 --transfer-bin boot/fs_impl.c --no-checksum
# netcat -U qemu_socket | stdbuf -o 0 hexdump -v -e '/1 "%02X "' # HEX MODE
# clear
