#!/bin/bash

# 1. Compile & Link
clang -target i686-none-elf -ffreestanding -c kernel.c -o kernel.o
ld.lld -m elf_i386 -T linker.ld boot.o kernel.o -o mykernel.bin

# 2. Auto Push to GitHub
if [ -n "$(git status --porcelain)" ]; then
    echo "[AUTO-SYNC] Changes detected. Pushing to GitHub..."
    git add .
    git commit -m "auto: live update with Real-Time Telegram Sentinel Engine $(date '+%Y-%m-%d %H:%M:%S')"
    git push origin main
    echo "[AUTO-SYNC] Successfully pushed to GitHub!"
fi

# 3. Clean previous instances & Launch Sentinel Monitor
killall -9 qemu-system-x86_64 2>/dev/null
echo "[BOOT] Launching Moksha Kernel with Real-Time Telegram Sentinel Bridge..."
python3 monitor_bridge.py
