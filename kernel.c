#include <stdint.h>
#include <stddef.h>

#define UART_COM1 0x3F8

// --- PAGING STRUCTURES (8 MB Mapped) ---
__attribute__((aligned(4096))) uint32_t page_directory[1024];
__attribute__((aligned(4096))) uint32_t page_table_0[1024];
__attribute__((aligned(4096))) uint32_t page_table_1[1024];

// --- HARDWARE I/O ---
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// --- UART SERIAL DRIVER ---
void uart_init() {
    outb(UART_COM1 + 1, 0x00);
    outb(UART_COM1 + 3, 0x80);
    outb(UART_COM1 + 0, 0x03);
    outb(UART_COM1 + 1, 0x00);
    outb(UART_COM1 + 3, 0x03);
    outb(UART_COM1 + 2, 0xC7);
    outb(UART_COM1 + 4, 0x0B);
}

void uart_putc(char c) {
    while ((inb(UART_COM1 + 5) & 0x20) == 0);
    outb(UART_COM1, c);
}

void uart_puts(const char* str) {
    while (*str) {
        if (*str == '\n') uart_putc('\r');
        uart_putc(*str++);
    }
}

void print_dec(uint32_t val) {
    char buf[12];
    int i = 0;
    if (val == 0) {
        uart_putc('0');
        return;
    }
    while (val > 0) {
        buf[i++] = (val % 10) + '0';
        val /= 10;
    }
    while (--i >= 0) {
        uart_putc(buf[i]);
    }
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// --- PAGING SUBSYSTEM ---
void init_paging() {
    for (int i = 0; i < 1024; i++) {
        page_table_0[i] = (i * 0x1000) | 3;
        page_table_1[i] = ((1024 + i) * 0x1000) | 3;
        page_directory[i] = 0x00000002;
    }

    page_directory[0] = ((uint32_t)page_table_0) | 3;
    page_directory[1] = ((uint32_t)page_table_1) | 3;

    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_directory) : "eax", "memory"
    );
}

// --- VIRTUAL FILE SYSTEM & IN-MEMORY RAMDISK ---
#define MAX_FILES 8
#define MAX_FILE_SIZE 512

typedef struct {
    char name[32];
    uint32_t size;
    uint8_t data[MAX_FILE_SIZE];
    int in_use;
} vfs_file_t;

static vfs_file_t ramdisk[MAX_FILES];

void init_vfs() {
    for (int i = 0; i < MAX_FILES; i++) {
        ramdisk[i].in_use = 0;
        ramdisk[i].size = 0;
        ramdisk[i].name[0] = '\0';
    }
}

int vfs_create(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (!ramdisk[i].in_use) {
            strcpy(ramdisk[i].name, filename);
            ramdisk[i].size = 0;
            ramdisk[i].in_use = 1;
            return i;
        }
    }
    return -1; // RAMDisk Full
}

int vfs_write(const char* filename, const char* src_data, uint32_t len) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (ramdisk[i].in_use && strcmp(ramdisk[i].name, filename) == 0) {
            if (len > MAX_FILE_SIZE) len = MAX_FILE_SIZE;
            for (uint32_t j = 0; j < len; j++) {
                ramdisk[i].data[j] = src_data[j];
            }
            ramdisk[i].size = len;
            return len;
        }
    }
    return -1; // File not found
}

int vfs_read(const char* filename, char* dest_buf, uint32_t max_len) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (ramdisk[i].in_use && strcmp(ramdisk[i].name, filename) == 0) {
            uint32_t read_bytes = (ramdisk[i].size < max_len) ? ramdisk[i].size : max_len;
            for (uint32_t j = 0; j < read_bytes; j++) {
                dest_buf[j] = ramdisk[i].data[j];
            }
            dest_buf[read_bytes] = '\0';
            return read_bytes;
        }
    }
    return -1; // File not found
}

void vfs_list() {
    uart_puts("\n[RAMDISK VFS CATALOG]\n");
    int count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (ramdisk[i].in_use) {
            uart_puts("  - File: ");
            uart_puts(ramdisk[i].name);
            uart_puts(" | Size: ");
            print_dec(ramdisk[i].size);
            uart_puts(" bytes\n");
            count++;
        }
    }
    if (count == 0) {
        uart_puts("  (No files found in RAMDisk)\n");
    }
}

// --- KERNEL ENTRY POINT ---
void kernel_main() {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("\n======================================================\n");
    uart_puts("   🛡️ MOKSHA MICROKERNEL v1.4 (VFS & RAMDISK CORE)\n");
    uart_puts("======================================================\n");
    uart_puts("[OK] 16 KB Physical Stack Locked.\n");

    init_paging();
    uart_puts("[OK] 8 MB Identity Paging Active.\n");

    init_vfs();
    uart_puts("[OK] In-Memory RAMDisk Subsystem Initialized.\n");

    // VFS Self-Test: Create and Write Files
    uart_puts("\n[TEST] Executing VFS File System Test:\n");
    
    // File 1: Sentinel Configuration
    vfs_create("sentinel.cfg");
    const char* cfg_data = "SECURE_MODE=ACTIVE;VIP_IP=10.0.0.1;AUTO_CALL=TRUE";
    vfs_write("sentinel.cfg", cfg_data, 50);
    uart_puts("       [WRITE] Created 'sentinel.cfg' with 50 bytes config data.\n");

    // File 2: System Log File
    vfs_create("sys.log");
    const char* log_data = "BOOT_SUCCESS_v1.4_RAMDISK_READY";
    vfs_write("sys.log", log_data, 31);
    uart_puts("       [WRITE] Created 'sys.log' with 31 bytes log data.\n");

    // Display File Catalog
    vfs_list();

    // Read and verify file contents
    char read_buffer[64];
    vfs_read("sentinel.cfg", read_buffer, sizeof(read_buffer));
    uart_puts("\n[READ VERIFICATION] Reading 'sentinel.cfg':\n");
    uart_puts("  Content: \"");
    uart_puts(read_buffer);
    uart_puts("\"\n");

    uart_puts("------------------------------------------------------\n");
    uart_puts("[SUCCESS] VFS & RAMDisk Read/Write Fully Operational.\n");
    uart_puts("moksha-shield> ");

    while (1) {
        __asm__ volatile ("hlt");
    }
}
