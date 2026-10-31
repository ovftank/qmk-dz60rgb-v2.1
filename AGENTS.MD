# AGENTS

when write code, follow the principle "keep it simple, stupid" and "you aren't gonna need it"

## comments

let naming and structure explain the code. don't fucking add comment description unnecessary to my code.

never:

- restate what code already says
- commented-out code or stale logic blocks
- conversational notes, obvious function headers, section divider markers

only when code can't say it itself, max 2 lines:

- why: intent, hidden constraint, non-obvious design decision
- safety warning: dangerous side effect, external platform quirk
- short url to source docs when copying complex external algorithm

## flashing

bootloader is LUFA mass storage (`KBDFANS` drive, 26624 B `.bin`). never `umount --force`: it drops unflushed page cache and leaves the app half-written, board stops enumerating.

the write path done right, in order:

1. kill nautilus/gvfs first (`nautilus -q`, `pkill -x gvfsd-metadata`, `pkill -x gvfsd-trash`). use `-x`, `-f` matches your own shell. killing them after mount unmounts the volume, so kill before mounting.
2. `gsettings set org.gnome.desktop.media-handling automount false`, then `udisksctl mount -b /dev/sdX`.
3. overwrite `FLASH.BIN` in place, never delete it: `dd if=fw.bin of=/media/$USER/KBDFANS/FLASH.BIN bs=512 conv=notrunc oflag=direct conv=fsync`. keep `EEPROM.BIN`. `rm` + new file changes its cluster layout, and the bootloader maps file LBA to flash address, so the app lands shifted (seen: +2048 B) or is not programmed at all.
4. verify the file: `dd if=/media/$USER/KBDFANS/FLASH.BIN bs=512 iflag=direct | md5sum` must match the source (`md5sum` alone reads page cache).
5. `udisksctl unmount`, wait ~10s, `udisksctl mount` again and hash `FLASH.BIN` once more: the drive mirrors the chip, so this checks the flash itself without a power cycle. then `udisksctl power-off`, wait, replug.
6. cutting power right off unmount leaves the app half-written, that is the "board stops enumerating" failure.

recover a board that stopped enumerating: hold `Esc` while plugging, or press the RESET button on the back of the PCB.
