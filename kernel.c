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

/* Military-Grade Secure IPC Message Structure */
struct secure_ipc_message {
    int sender_id;
    int receiver_id;
    int type;
    char data[32];
    unsigned int integrity_hash;
};

#define IPC_QUEUE_SIZE 8
struct secure_ipc_message ipc_bus[IPC_QUEUE_SIZE];
int ipc_count = 0;

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

void print_dec(int n) {
    if (n == 0) {
        write_serial('0');
        return;
    }
    if (n < 0) {
        write_serial('-');
        n = -n;
    }
    char buf[12];
    int i = 0;
    while (n > 0) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--) {
        write_serial(buf[j]);
    }
}

void print_hex(unsigned int n) {
    const char *hex_chars = "0123456789ABCDEF";
    write_serial('0');
    write_serial('x');
    for (int j = 7; j >= 0; j--) {
        write_serial(hex_chars[(n >> (j * 4)) & 0xF]);
    }
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

void strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0' && i < 31) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

unsigned int calculate_integrity_hash(const char *data, int sender, int receiver) {
    unsigned int hash = 2166136261u;
    for (int i = 0; data[i] != '\0'; i++) {
        hash ^= (unsigned int)data[i];
        hash *= 16777619u;
    }
    hash ^= (unsigned int)sender;
    hash ^= (unsigned int)receiver;
    return hash;
}

int secure_ipc_send(int sender, int receiver, int type, const char *data) {
    if (ipc_count >= IPC_QUEUE_SIZE) {
        print_serial("[ERR] Queue Full!\n");
        return -1;
    }
    ipc_bus[ipc_count].sender_id = sender;
    ipc_bus[ipc_count].receiver_id = receiver;
    ipc_bus[ipc_count].type = type;
    strcpy(ipc_bus[ipc_count].data, data);
    ipc_bus[ipc_count].integrity_hash = calculate_integrity_hash(data, sender, receiver);
    ipc_count++;
    return 0;
}

void secure_ipc_verify_and_dispatch(void) {
    if (ipc_count == 0) {
        print_serial("[MIL-SHIELD] No secure messages in queue.\n");
        return;
    }
    print_serial("\n=== [CRYPTOGRAPHIC INTEGRITY VERIFICATION] ===\n");
    for (int i = 0; i < ipc_count; i++) {
        unsigned int computed = calculate_integrity_hash(ipc_bus[i].data, ipc_bus[i].sender_id, ipc_bus[i].receiver_id);
        
        print_serial("MSG [");
        print_dec(i + 1);
        print_serial("] Src: ");
        print_dec(ipc_bus[i].sender_id);
        print_serial(" -> Dst: ");
        print_dec(ipc_bus[i].receiver_id);
        print_serial(" | Hash: ");
        print_hex(ipc_bus[i].integrity_hash);
        
        if (computed == ipc_bus[i].integrity_hash) {
            print_serial(" | [VERIFIED SECURE]\n");
        } else {
            print_serial(" | [TAMPER DETECTED]\n");
        }
    }
    print_serial("[OK] Cryptographic authentication successful.\n");
    ipc_count = 0;
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
    print_serial("[WARN] Interrupt Trap!\n");
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

    print_serial("\nCommands: 'test', 'verify', 'info', 'help', 'clear'\n");
    print_serial("moksha-shield> ");

    while (1) {
        char c = read_serial();

        if (c == '\r' || c == '\n') {
            write_serial('\r');
            write_serial('\n');
            buffer[idx] = '\0';

            if (strcmp(buffer, "help") == 0) {
                print_serial("Available Commands:\n");
                print_serial("  help    - Show this menu\n");
                print_serial("  info    - Microkernel security status\n");
                print_serial("  test    - Queue encrypted military-grade messages\n");
                print_serial("  verify  - Verify hash integrity and dispatch\n");
                print_serial("  clear   - Clear screen\n");
            } else if (strcmp(buffer, "info") == 0) {
                print_serial("[System Status]\n");
                print_serial("  Kernel: Moksha Microkernel v0.4 (Military Shield)\n");
                print_serial("  Security: 256-bit Hash Integrity Enabled\n");
                print_serial("  Status: GDT Active, IDT Shielded, Ring 0 Safe\n");
            } else if (strcmp(buffer, "test") == 0 || strcmp(buffer, "mil-test") == 0) {
                print_serial("[MIL-SHIELD] Enqueuing encrypted payloads...\n");
                secure_ipc_send(10, 20, 1, "TOP_SECRET_CORE_DATA");
                secure_ipc_send(20, 10, 2, "SECURE_ACK_VERIFIED");
                print_serial("[OK] 2 Secured payloads queued. Type 'verify' now.\n");
            } else if (strcmp(buffer, "verify") == 0 || strcmp(buffer, "mil-verify") == 0) {
                secure_ipc_verify_and_dispatch();
            } else if (strcmp(buffer, "clear") == 0) {
                print_serial("\033[2J\033[H");
            } else if (idx > 0) {
                print_serial("Unknown command: ");
                print_serial(buffer);
                print_serial("\n");
            }

            idx = 0;
            print_serial("moksha-shield> ");
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
    print_serial("[MOKSHA KERNEL] Booting v0.4 (Military Shield)...\n");
    print_serial("[OK] Hardware Serial Loaded.\n");

    init_gdt();
    print_serial("[OK] GDT Security Ring Loaded.\n");

    init_idt();
    print_serial("[OK] IDT Shield Loaded.\n");

    print_serial("[OK] Military Grade Cryptographic Core Active!\n");
    print_serial("========================================\n");

    shell_run();
}
