🛡️ MOKSHA MICROKERNEL (CYBER-PHYSICAL SENTINEL CORE)
Lead Architect: Moksha
Architecture: x86 32-bit Protected Mode (i686-none-elf)
Build Platform: Termux (Android Environment)
Virtualization Target: QEMU (qemu-system-x86_64)
Bootable Medium: Standard ISO 9660 Image (moksha-os.iso - 382 KB)
Toolchain: NASM, Clang / GCC (-m32 -ffreestanding), LLD / LD, Python 3, xorriso
Telemetry Bridge: ntfy.sh Sentinel Real-Time Telemetry (Moksha-Master-Core)
Repository: https://github.com/MOKSHA2025/moksha-kernel⁠�
Current Status: v1.4 COMPLETE | PRODUCTION LOCKED | BOOTABLE ISO VERIFIED
MOKSHA MICROKERNEL is a standalone x86 kernel project focused on low-level system architecture, memory management, hardware I/O, interrupt handling, multitasking, virtual file systems, and a cyber-physical sentinel interface.
The project has been intentionally finalized at v1.4. No further feature development is planned for this project release.
TECHNICAL ARCHITECTURE & MEMORY LAYOUT
The kernel operates around an 8 MB managed memory layout.
0x00000000 to 0x000FFFFF (1 MB): Low Memory, Real-Mode IVT & BIOS Area.
0x00100000 to 0x001FFFFF (1 MB): Kernel Core Binary, including Code, Data, GDT, IDT, and 16 KB Stack.
0x00200000 to 0x002FFFFF (1 MB): Dynamic Kernel Heap Pool used by kmalloc and kfree.
0x00300000 to 0x007FFFFF (5 MB): Extended Kernel Area containing the in-memory VFS RAMDisk and multitasking task frames.
SUBSYSTEM EVOLUTION LOG
v0.1 - FOUNDATION & MULTIBOOT LOADER
GRUB-compliant boot.s Multiboot header.
16 KB dedicated kernel execution stack.
32-bit Protected Mode initialization.
CPU register initialization.
1 MB kernel base boundary defined through linker.ld.
v0.2 - LOW-LEVEL I/O & UART SERIAL DRIVER
Inline hardware port I/O routines using outb and inb.
Bare-metal memcpy and strcmp utilities.
UART COM1 serial controller.
38,400 baud serial communication.
ANSI escape sequence parsing.
v0.3 - RING 0 PROTECTION & GDT
Global Descriptor Table.
4 GB flat memory model.
Kernel Code Segment at 0x08.
Kernel Data Segment at 0x10.
GDT loading through lgdt.
v0.4 - SHIELD CORE & IDT
Cryptographic payload verification stub for IPC packets.
Interrupt Descriptor Table.
Trap gate configuration.
Interactive moksha-shield> shell loop.
PS/2 hardware reset through 0xFE to port 0x64.
v0.5 - STATEFUL QUARANTINE & GATEKEEPER
Dynamic quarantine state.
has_pending_request state tracking.
is_locked_down state tracking.
VIP subnet fast-track simulation.
10.0.0.1 trusted-path simulation.
Cryptographic Visitor Pass generation.
Visitor Pass identifier: #VP-9982.
v0.6 - HEURISTIC INTEGRITY ENGINE
Embedded ai_manager_analyze heuristic engine.
Ring 0 integrity monitoring.
Intruder risk assessment.
Nominal baseline risk calculation.
Telemetry analysis.
The engine is a heuristic integrity monitor rather than a machine-learning model.
v0.7 - RAW KEYSTREAM & SERIAL DECODER
Real-time keyboard event inspection.
key-test debug mode.
Keyboard scan code extraction.
Hexadecimal scan-code formatting through print_hex.
v0.8 - PTY TELEMETRY BRIDGE
Python-based PTY bridge.
pty.openpty integration.
Raw serial traffic filtering.
Host-side socket communication.
Asynchronous telemetry processing.
v0.9 - TELEMETRY PUSH PIPELINE
Full-duplex telemetry pipeline.
ntfy.sh integration.
Moksha-Master-Core telemetry channel.
Priority-based alert mapping.
Urgent and High alert states.
v1.0 - 8 MB IDENTITY PAGING
Page Directory.
Page Tables.
CR0 and CR3 paging control.
Ring 0 memory enforcement.
16 KB kernel stack allocation.
v1.1 - DYNAMIC MEMORY ALLOCATOR
1 MB kernel heap.
Heap base at 0x00200000.
8-byte aligned kmalloc allocation.
kfree memory release.
Heap block coalescing.
v1.2 - INTERACTIVE SHELL CORE
Dynamic moksha-shield> command interpreter.
UART input buffering.
Backspace handling.
Runtime command execution.
v1.3 - PREEMPTIVE MULTITASKING
Task Control Block scheduler.
4 KB isolated task stacks.
Round-robin scheduling.
Task switching.
Sentinel Pulse task.
Heap Watchdog task.
v1.4 - VIRTUAL FILE SYSTEM & RAMDISK
In-memory RAMDisk.
Dynamic file descriptors.
VFS abstraction.
vfs_create operation.
vfs_write operation.
vfs_read operation.
vfs_list operation.
FINAL v1.4 RELEASE
The v1.4 milestone is the final development state of MOKSHA MICROKERNEL.
The project is intentionally production locked. The current architecture and feature set represent the finalized scope of this project.
No further feature development is planned after v1.4.
FINAL RELEASE COMPONENTS
x86 32-bit Protected Mode kernel.
GDT and IDT architecture.
UART serial communication.
Keyboard input inspection.
8 MB paging architecture.
Dynamic kernel heap.
Interactive sentinel shell.
Ring 0 integrity monitoring.
Quarantine and lockdown simulation.
Preemptive multitasking.
RAMDisk-based VFS.
Host-side telemetry bridge.
Bootable ISO image.
Final Version: v1.4
Release State: Production Locked
Boot Medium: moksha-os.iso
ISO Size: 382 KB
COMMAND MATRIX
help
Displays the sentinel command index and syntax guide.
ai-status
Executes the heuristic integrity scan and outputs Ring 0 integrity status.
mem-info
Displays the 8 MB paging map and kernel heap information.
alloc
Dynamically allocates a 64-byte block through kmalloc.
free
Releases dynamically allocated memory through kfree.
key-test
Enters keyboard driver debug mode and captures raw scan bytes.
admin-req
Simulates VIP fast-track authentication from the configured trusted path.
unknown-req
Quarantines an untrusted request and triggers the telemetry pipeline.
approve
Generates Visitor Pass #VP-9982 and clears the quarantine state.
deny
Blacklists the request and activates the Ring 0 lockdown state.
reset
Acts as the master override to restore the normal gate state.
clear
Clears the terminal screen buffer.
reboot
Sends the PS/2 controller reset command.
BUILD & EMULATION
Compile the bootloader using NASM.
nasm -f elf32 boot.s -o boot.o
Compile the freestanding kernel using the i686-none-elf target.
clang -target i686-none-elf -ffreestanding -m32 -c kernel.c -o kernel.o
Link the kernel using the i386 linker target.
ld.lld -m elf_i386 -T linker.ld -o mykernel.bin boot.o kernel.o
Run the bootable image under QEMU.
qemu-system-x86_64 -cdrom moksha-os.iso -kernel mykernel.bin -serial stdio -display none
PROJECT STRUCTURE
moksha-kernel
boot.s
boot.o
kernel.c
kernel.o
linker.ld
moksha-os.iso
PROJECT_LOG.txt
index.html
isodir/boot
ARCHITECTURE PHILOSOPHY
MOKSHA MICROKERNEL follows an incremental low-level architecture model.
BOOT
↓
PROTECTED MODE
↓
GDT AND IDT
↓
PAGING
↓
MEMORY MANAGEMENT
↓
HARDWARE I/O
↓
SCHEDULER
↓
VFS
↓
SENTINEL CONTROL LAYER
↓
TELEMETRY
Each subsystem was introduced incrementally, evolving the project from a basic bootable kernel foundation into the finalized v1.4 Cyber-Physical Sentinel Core.
FINAL STATUS
MOKSHA MICROKERNEL v1.4
STATUS: PRODUCTION LOCKED
STATE: FINAL DEVELOPMENT MILESTONE
ARCHITECTURE: x86 32-BIT PROTECTED MODE
MEMORY MODEL: 8 MB MANAGED ADDRESS SPACE
BOOT FORMAT: ISO 9660 / GRUB
VIRTUALIZATION TARGET: QEMU
DEVELOPMENT STATUS: COMPLETE
FINAL RELEASE: v1.4
MOKSHA MICROKERNEL v1.4 — FINAL DEVELOPMENT MILESTONE.
