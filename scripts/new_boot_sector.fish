#!/usr/bin/env fish
mkdir -p out
rm -f out/new_boot_sector.bin
nasm -w+all -w+error -w-reloc -f bin boot/new_boot_sector.asm -o out/new_boot_sector.bin; and \
cargo run --manifest-path bin_to_code/Cargo.toml -- bin-to-code out/new_boot_sector.bin tmp
