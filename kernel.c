#include <stdint.h>
#include <stddef.h>

#define UART_COM1 0x3F8

// --- PAGING STRUCTURES (8 MB Mapped) ---
__attribute__((aligned(4096))) uint32_t page_directory[1024];
__attribute__((aligned(4096))) uint32_t page_table_0[1024];
__attribute__((aligned(4096))) uint32_t page_table_1[1024];

// --- HEAP REGION ---
#define HEAP_START 0x00200000
#define HEAP_SIZE  0x00100000

typedef struct block_header {
    size_t size;
    int is_free;
    struct block_header* next;
} block_header_t;

static block_header_t* free_list = (block_header_t*)HEAP_START;
static void* test_dynamic_ptr = NULL;

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

char uart_getc() {
    while ((inb(UART_COM1 + 5) & 1) == 0);
    return inb(UART_COM1);
}

void print_hex(uint32_t val) {
    const char* hex_digits = "0123456789ABCDEF";
    uart_puts("0x");
    for (int i = 7; i >= 0; i--) {
        uart_putc(hex_digits[(val >> (i * 4)) & 0xF]);
    }
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
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

// --- HEAP ALLOCATOR ---
void init_heap() {
    free_list->size = HEAP_SIZE - sizeof(block_header_t);
    free_list->is_free = 1;
    free_list->next = NULL;
}

void* kmalloc(size_t size) {
    size = (size + 7) & ~7;
    block_header_t* curr = free_list;

    while (curr) {
        if (curr->is_free && curr->size >= size) {
            if (curr->size >= size + sizeof(block_header_t) + 8) {
                block_header_t* next_block = (block_header_t*)((uint8_t*)curr + sizeof(block_header_t) + size);
                next_block->size = curr->size - size - sizeof(block_header_t);
                next_block->is_free = 1;
                next_block->next = curr->next;

                curr->size = size;
                curr->next = next_block;
            }
            curr->is_free = 0;
            return (void*)((uint8_t*)curr + sizeof(block_header_t));
        }
        curr = curr->next;
    }
    return NULL;
}

void kfree(void* ptr) {
    if (!ptr) return;
    block_header_t* header = (block_header_t*)((uint8_t*)ptr - sizeof(block_header_t));
    header->is_free = 1;

    block_header_t* curr = free_list;
    while (curr && curr->next) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(block_header_t) + curr->next->size;
            curr->next = curr->next->next;
        }
        curr = curr->next;
    }
}

// --- COMMAND PARSER ---
void execute_command(char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        uart_puts("\nAvailable Commands:\n");
        uart_puts("  help      - List all shell commands\n");
        uart_puts("  mem-info  - Display paging & dynamic heap status\n");
        uart_puts("  alloc     - Dynamically allocate 64-byte block (kmalloc)\n");
        uart_puts("  free      - Release allocated memory block (kfree)\n");
        uart_puts("  clear     - Clear terminal display\n");
        uart_puts("  reboot    - Trigger x86 hardware reset\n");
    } else if (strcmp(cmd, "mem-info") == 0) {
        uart_puts("\n[MEMORY MAP STATUS]\n");
        uart_puts("  Identity Paging : 8 MB Active (CR0/CR3)\n");
        uart_puts("  Kernel Heap Base: "); print_hex(HEAP_START); uart_puts("\n");
        uart_puts("  Total Heap Pool : 1024 KB\n");
    } else if (strcmp(cmd, "alloc") == 0) {
        if (test_dynamic_ptr != NULL) {
            uart_puts("\n[WARN] Memory block already allocated at ");
            print_hex((uint32_t)test_dynamic_ptr);
            uart_puts(". Run 'free' first.\n");
        } else {
            test_dynamic_ptr = kmalloc(64);
            uart_puts("\n[OK] Allocated 64 bytes via kmalloc() -> Base: ");
            print_hex((uint32_t)test_dynamic_ptr);
            uart_puts("\n");
        }
    } else if (strcmp(cmd, "free") == 0) {
        if (test_dynamic_ptr == NULL) {
            uart_puts("\n[WARN] No dynamic memory currently allocated.\n");
        } else {
            kfree(test_dynamic_ptr);
            uart_puts("\n[OK] Block at ");
            print_hex((uint32_t)test_dynamic_ptr);
            uart_puts(" successfully released via kfree().\n");
            test_dynamic_ptr = NULL;
        }
    } else if (strcmp(cmd, "clear") == 0) {
        uart_puts("\033[2J\033[H");
    } else if (strcmp(cmd, "reboot") == 0) {
        uart_puts("\n[SYSTEM] Triggering hardware reset...\n");
        outb(0x64, 0xFE);
    } else if (cmd[0] != '\0') {
        uart_puts("\n[ERROR] Unknown command: '");
        uart_puts(cmd);
        uart_puts("'. Type 'help' for valid commands.\n");
    }
}

// --- KERNEL ENTRY POINT & SHELL LOOP ---
void kernel_main() {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("\n======================================================\n");
    uart_puts("   🛡️ MOKSHA MICROKERNEL v1.2 (INTERACTIVE SHELL CORE)\n");
    uart_puts("======================================================\n");
    uart_puts("[OK] 16 KB Physical Stack Locked.\n");

    init_paging();
    uart_puts("[OK] 8 MB Identity Paging Active.\n");

    init_heap();
    uart_puts("[OK] Dynamic Heap Allocator Ready.\n");
    uart_puts("Type 'help' to inspect kernel commands.\n");
    uart_puts("------------------------------------------------------\n");

    char buffer[64];
    int idx = 0;

    while (1) {
        uart_puts("moksha-shield> ");
        idx = 0;
        
        while (1) {
            char c = uart_getc();

            if (c == '\r' || c == '\n') {
                uart_putc('\r');
                uart_putc('\n');
                buffer[idx] = '\0';
                break;
            } else if (c == 0x08 || c == 0x7F) { // Backspace
                if (idx > 0) {
                    idx--;
                    uart_puts("\b \b");
                }
            } else if (idx < 63 && c >= 32 && c <= 126) {
                buffer[idx++] = c;
                uart_putc(c);
            }
        }

        execute_command(buffer);
    }
}
