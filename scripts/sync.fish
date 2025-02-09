#!/usr/bin/env fish
if cargo build --manifest-path utils/Cargo.toml
  sudo ./utils/target/debug/utils sync-programs qemu.img programs/
end
