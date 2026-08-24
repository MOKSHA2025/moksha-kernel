#define PORT 0x3f8

/* GDT Structures */
struct gdt_entry_struct {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char  base_middle;
    unsigned char  access;
    unsigned char  granularity;
    unsigned char  base_high;
} __attribute__((packed));

struct gdt_ptr_struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct gdt_entry_struct gdt_entries[3];
struct gdt_ptr_struct   gdt_ptr;

/* IDT Structures */
struct idt_entry_struct {
    unsigned short base_low;
    unsigned short sel;
    unsigned char  always0;
    unsigned char  flags;
    unsigned short base_high;
} __attribute__((packed));

struct idt_ptr_struct {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct idt_entry_struct idt_entries[256];
struct idt_ptr_struct   idt_ptr;

static inline void outb(unsigned short port, unsigned char val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline unsigned char inb(unsigned short port) {
    unsigned char ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void init_serial(void) {
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x80);
    outb(PORT + 0, 0x03);
    outb(PORT + 1, 0x00);
    outb(PORT + 3, 0x03);
    outb(PORT + 2, 0xC7);
    outb(PORT + 4, 0x0B);
}

int serial_received(void) {
    return inb(PORT + 5) & 1;
}

char read_serial(void) {
    while (serial_received() == 0);
    return inb(PORT);
}

void write_serial(char a) {
    while ((inb(PORT + 5) & 0x20) == 0);
    outb(PORT, a);
}

void print_serial(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void gdt_set_gate(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
    gdt_entries[num].base_low = (base & 0xFFFF);
    gdt_entries[num].base_middle = (base >> 16) & 0xFF;
    gdt_entries[num].base_high = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low = (limit & 0xFFFF);
    gdt_entries[num].granularity = (limit >> 16) & 0x0F;
    gdt_entries[num].granularity |= gran & 0xF0;
    gdt_entries[num].access = access;
}

void init_gdt(void) {
    gdt_ptr.limit = (sizeof(struct gdt_entry_struct) * 3) - 1;
    gdt_ptr.base  = (unsigned int)&gdt_entries;

    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    __asm__ __volatile__("lgdt (%0)" : : "r" (&gdt_ptr));
}

void idt_set_gate(unsigned char num, unsigned long base, unsigned short sel, unsigned char flags) {
    idt_entries[num].base_low = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    idt_entries[num].flags   = flags;
}

void default_interrupt_handler(void) {
    print_serial("[WARN] Unhandled Interrupt!\n");
}

void init_idt(void) {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (unsigned int)&idt_entries;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (unsigned long)default_interrupt_handler, 0x08, 0x8E);
    }

    __asm__ __volatile__("lidt (%0)" : : "r" (&idt_ptr));
}

void shell_run(void) {
    char buffer[64];
    int idx = 0;

    print_serial("\nType 'help', 'info', or 'clear' to interact with Moksha Shell.\n");
    print_serial("moksha-os> ");

    while (1) {
        char c = read_serial();

        if (c == '\r' || c == '\n') {
            write_serial('\r');
            write_serial('\n');
            buffer[idx] = '\0';

            if (strcmp(buffer, "help") == 0) {
                print_serial("Available Commands:\n");
                print_serial("  help  - Show this help message\n");
                print_serial("  info  - Display microkernel architecture stats\n");
                print_serial("  clear - Clear the terminal line\n");
            } else if (strcmp(buffer, "info") == 0) {
                print_serial("[System Info]\n");
                print_serial("  Kernel: Moksha Microkernel (v0.1)\n");
                print_serial("  Arch: x86 Bare-Metal (Protected Mode)\n");
                print_serial("  Status: GDT Active, IDT Active, I/O Active\n");
            } else if (strcmp(buffer, "clear") == 0) {
                print_serial("\033[2J\033[H");
            } else if (idx > 0) {
                print_serial("Unknown command: ");
                print_serial(buffer);
                print_serial("\n");
            }

            idx = 0;
            print_serial("moksha-os> ");
        } else if (c == 0x7F || c == '\b') {
            if (idx > 0) {
                idx--;
                print_serial("\b \b");
            }
        } else if (idx < 63) {
            buffer[idx++] = c;
            write_serial(c);
        }
    }
}

void kernel_main(void) {
    init_serial();
    print_serial("\n========================================\n");
    print_serial("[MOKSHA KERNEL] Booting Microkernel...\n");
    print_serial("[OK] Serial Driver Loaded.\n");

    init_gdt();
    print_serial("[OK] GDT (Global Descriptor Table) Loaded.\n");

    init_idt();
    print_serial("[OK] IDT (Interrupt Descriptor Table) Loaded.\n");

    print_serial("[OK] Microkernel Core Online!\n");
    print_serial("========================================\n");

    shell_run();
}
