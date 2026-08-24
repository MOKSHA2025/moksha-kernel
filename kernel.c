typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define COM1 0x3F8
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void init_serial(void) {
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x80);
    outb(COM1 + 0, 0x03);
    outb(COM1 + 1, 0x00);
    outb(COM1 + 3, 0x03);
    outb(COM1 + 2, 0xC7);
    outb(COM1 + 4, 0x0B);
}

int is_transmit_empty(void) {
    return inb(COM1 + 5) & 0x20;
}

void write_serial(char a) {
    while (is_transmit_empty() == 0);
    outb(COM1, a);
}

void print_serial(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        write_serial(str[i]);
    }
}

int serial_received(void) {
    return inb(COM1 + 5) & 1;
}

char read_serial(void) {
    while (serial_received() == 0);
    return inb(COM1);
}

/* GDT Structure */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gp;

void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= (gran & 0xF0);
    gdt[num].access = access;
}

void init_gdt(void) {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base = (uint32_t)&gdt;
    gdt_set_gate(0, 0, 0, 0, 0);
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    asm volatile ("lgdt (%0)" : : "r" (&gp));
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int has_pending_request = 0;
int is_locked_down = 0;

void print_hex(uint8_t val) {
    const char hex_chars[] = "0123456789ABCDEF";
    write_serial(hex_chars[(val >> 4) & 0x0F]);
    write_serial(hex_chars[val & 0x0F]);
}

void kernel_main(void) {
    init_serial();
    init_gdt();

    print_serial("\033[2J\033[H");
    print_serial("=====================================================\r\n");
    print_serial("       MOKSHA MICROKERNEL v0.7 (KEYBOARD DRIVER CORE)\r\n");
    print_serial("=====================================================\r\n");
    print_serial("[KEY DRIVER] Unified Serial & Key Stream Decoder: READY\r\n");
    print_serial("[VIP ENGINE] Fast-Track 10.0.0.1: ACTIVE\r\n");
    print_serial("[MCD DECK] Remote 2-Way Mobile Channel: READY\r\n");
    print_serial("Type 'help' for commands list.\r\n\r\n");

    char buffer[128];
    int buf_idx = 0;

    while (1) {
        print_serial("moksha-sentinel> ");
        buf_idx = 0;
        
        while (1) {
            char c = read_serial();
            
            if (c == '\r' || c == '\n') {
                buffer[buf_idx] = '\0';
                print_serial("\r\n");
                break;
            } else if (c == '\b' || c == 127) {
                if (buf_idx > 0) {
                    buf_idx--;
                    print_serial("\b \b");
                }
            } else if (buf_idx < 127) {
                buffer[buf_idx++] = c;
                write_serial(c);
            }
        }

        if (is_locked_down && strcmp(buffer, "reset") != 0) {
            print_serial("🚨 [ACCESS DENIED] System is under FULL LOCKDOWN!\r\n");
            continue;
        }

        if (strcmp(buffer, "help") == 0) {
            print_serial("Available Commands:\r\n");
            print_serial("  key-test     - Live raw key event decoder & hex inspector\r\n");
            print_serial("  admin-req    - Fast-track verified 10.0.0.1 request\r\n");
            print_serial("  unknown-req  - Ingest quarantine packet & alert MCD\r\n");
            print_serial("  approve      - Authorize quarantined IP\r\n");
            print_serial("  deny         - Neutralize intruder & lockdown gates\r\n");
            print_serial("  reset        - Lift emergency lockdown\r\n");
            print_serial("  reboot       - Trigger hardware reboot\r\n");
        } else if (strcmp(buffer, "key-test") == 0) {
            print_serial("[KEYBOARD DRIVER TEST ACTIVE]\r\n");
            print_serial("Press any key to inspect raw events (Press 'q' or Enter to stop):\r\n");
            while (1) {
                char ch = read_serial();
                if (ch == 'q' || ch == '\r' || ch == '\n') {
                    print_serial("\r\n[KEYBOARD TEST FINISHED]\r\n");
                    break;
                }
                print_serial(" -> Event Captured: '");
                write_serial(ch);
                print_serial("' | Byte: 0x");
                print_hex((uint8_t)ch);
                print_serial(" | Decimal: ");
                
                uint8_t d = (uint8_t)ch;
                if (d >= 100) write_serial('0' + (d / 100));
                if (d >= 10) write_serial('0' + ((d / 10) % 10));
                write_serial('0' + (d % 10));
                print_serial("\r\n");
            }
        } else if (strcmp(buffer, "admin-req") == 0) {
            print_serial(">>> [VIP FAST-TRACK AUTHENTICATED] Source: 10.0.0.1 -> EXECUTE SUCCESS!\r\n");
        } else if (strcmp(buffer, "unknown-req") == 0) {
            has_pending_request = 1;
            print_serial("⚠️ [SENTINEL ALERT -> NOTIFICATION TO MOKSHA MASTER PHONE]\r\n");
            print_serial("Suspicious IP 192.168.1.108 held in quarantine.\r\n");
        } else if (strcmp(buffer, "approve") == 0) {
            if (has_pending_request) {
                has_pending_request = 0;
                print_serial("🛡️ [MOKSHA APPROVED] Visitor Pass Generated: #VP-9982\r\n");
            } else {
                print_serial("[INFO] No pending requests to approve.\r\n");
            }
        } else if (strcmp(buffer, "deny") == 0) {
            if (has_pending_request) {
                has_pending_request = 0;
                is_locked_down = 1;
                print_serial("🚨 [MOKSHA REJECTED] INTRUDER BLACKLISTED! EMERGENCY LOCKDOWN!\r\n");
            } else {
                print_serial("[INFO] No pending requests to deny.\r\n");
            }
        } else if (strcmp(buffer, "reset") == 0) {
            is_locked_down = 0;
            print_serial("🔓 [MASTER OVERRIDE] Lockdown Lifted. Gates Normal.\r\n");
        } else if (strcmp(buffer, "reboot") == 0) {
            outb(0x64, 0xFE);
        } else if (buf_idx > 0) {
            print_serial("Unknown command. Type 'help'.\r\n");
        }
    }
}
