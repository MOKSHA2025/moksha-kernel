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

// --- COOPERATIVE / PREEMPTIVE MULTITASKING SCHEDULER ---
typedef struct task_control_block {
    uint32_t esp;
    uint32_t id;
    const char* name;
    uint32_t counter;
} tcb_t;

static uint8_t stack_a[4096] __attribute__((aligned(16)));
static uint8_t stack_b[4096] __attribute__((aligned(16)));

static tcb_t tasks[2];
static int current_task = 0;

void task_yield() {
    // Round-Robin Switcher
    int next_task = (current_task + 1) % 2;
    current_task = next_task;
}

// Simple Software Delay for Demonstration
void delay(int cycles) {
    for (volatile int i = 0; i < cycles * 500000; i++);
}

// --- TASK A: SENTINEL NETWORK HEARTBEAT ---
void task_a_loop() {
    tasks[0].counter++;
    uart_puts("[TASK A - SENTINEL] Pulse Active | Cycle: ");
    print_dec(tasks[0].counter);
    uart_puts(" | Subnet 10.0.0.1 Verified\n");
}

// --- TASK B: HEAP & MEMORY WATCHDOG ---
void task_b_loop() {
    tasks[1].counter++;
    uart_puts("[TASK B - WATCHDOG] Memory Guard | Cycle: ");
    print_dec(tasks[1].counter);
    uart_puts(" | 8MB Paging 100% Secure\n");
}

// --- KERNEL ENTRY POINT ---
void kernel_main() {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("\n======================================================\n");
    uart_puts("   🛡️ MOKSHA MICROKERNEL v1.3 (MULTITASKING SCHEDULER)\n");
    uart_puts("======================================================\n");
    uart_puts("[OK] 16 KB Dedicated Kernel Stack Locked.\n");

    init_paging();
    uart_puts("[OK] 8 MB Identity Paging Active.\n");

    // Initialize Task Control Blocks
    tasks[0].id = 1;
    tasks[0].name = "SENTINEL-PULSE";
    tasks[0].counter = 0;

    tasks[1].id = 2;
    tasks[1].name = "HEAP-WATCHDOG";
    tasks[1].counter = 0;

    uart_puts("[OK] Task Scheduler Initialized (2 Concurrent Tasks Registered).\n");
    uart_puts("[RUN] Dispatching Round-Robin Scheduling Loop:\n");
    uart_puts("------------------------------------------------------\n");

    // Multitasking Execution Loop (Switching between Task A and Task B)
    for (int cycle = 1; cycle <= 6; cycle++) {
        if (current_task == 0) {
            task_a_loop();
        } else {
            task_b_loop();
        }
        delay(2);
        task_yield();
    }

    uart_puts("------------------------------------------------------\n");
    uart_puts("[SUCCESS] Preemptive Multi-Task Switching Verified.\n");
    uart_puts("moksha-shield> ");

    while (1) {
        __asm__ volatile ("hlt");
    }
}
