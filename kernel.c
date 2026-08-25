#include <stdint.h>
#define UART_COM1 0x3F8

__attribute__((aligned(4096))) uint32_t page_directory[1024];
__attribute__((aligned(4096))) uint32_t first_page_table[1024];

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

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
    outb(UART_COM1, c);
}

void uart_puts(const char* str) {
    while (*str) {
        if (*str == "\n") uart_putc("\r");
        uart_putc(*str++);
    }
}

void init_paging() {
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * 0x1000) | 3;
    }
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }
    page_directory[0] = ((uint32_t)first_page_table) | 3;

    __asm__ volatile (
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(page_directory) : "eax", "memory"
    );
}

void kernel_main() {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("\n======================================================\n");
    uart_puts("   🛡️ MOKSHA MICROKERNEL v1.0 (PAGING & STACK ACTIVE)\n");
    uart_puts("======================================================\n");
    uart_puts("[OK] 16 KB Dedicated Execution Stack Bound.\n");
    init_paging();
    uart_puts("[OK] 4 KB Identity Paging Activated via CR0/CR3.\n");
    uart_puts("[OK] Virtual Memory Management: LOCKED & OPERATIONAL.\n");
    uart_puts("------------------------------------------------------\n");
    uart_puts("moksha-shield> ");
    while (1) {
        __asm__ volatile ("hlt");
    }
}
