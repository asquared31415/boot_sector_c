#!/usr/bin/env fish
set lodev (losetup --partscan --direct-io=on --nooverlap --show --find qemu_img)
if test $status -eq 0
  mount --mkdir --read-write (string join '' $lodev "p1") /mnt/qemu -o sync,umask=0000
else
  echo "failed to set up loop device $lodev"
end
