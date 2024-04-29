#!/usr/bin/env fish
echo $argv
gdb -ix "scripts/gdb_init_real_mode.txt" -ex "target remote :1234" -ex "c" $argv
