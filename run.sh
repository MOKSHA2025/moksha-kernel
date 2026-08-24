#!/bin/bash

# 1. Compile and Link
clang -target i686-none-elf -ffreestanding -c kernel.c -o kernel.o
ld.lld -m elf_i386 -T linker.ld boot.o kernel.o -o mykernel.bin

# 2. Auto Push to GitHub if changes exist
if [ -n "$(git status --porcelain)" ]; then
    echo "[AUTO-SYNC] Changes detected. Pushing to GitHub..."
    git add .
    git commit -m "auto: live update Moksha Kernel $(date '+%Y-%m-%d %H:%M:%S')"
    git push origin main
    echo "[AUTO-SYNC] Successfully pushed to GitHub!"
else
    echo "[AUTO-SYNC] No changes to push."
fi

# 3. Boot Kernel in QEMU
echo "[BOOT] Launching Moksha Kernel..."
qemu-system-x86_64 -kernel mykernel.bin -display none -serial stdio
