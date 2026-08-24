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

/* PS/2 Keyboard Scancode Set 1 Mapper */
char scancode_to_ascii(uint8_t scancode) {
    switch (scancode) {
        case 0x1E: return 'a'; case 0x30: return 'b'; case 0x2E: return 'c';
        case 0x20: return 'd'; case 0x12: return 'e'; case 0x21: return 'f';
        case 0x22: return 'g'; case 0x23: return 'h'; case 0x17: return 'i';
        case 0x24: return 'j'; case 0x25: return 'k'; case 0x26: return 'l';
        case 0x32: return 'm'; case 0x31: return 'n'; case 0x18: return 'o';
        case 0x19: return 'p'; case 0x10: return 'q'; case 0x13: return 'r';
        case 0x1F: return 's'; case 0x14: return 't'; case 0x16: return 'u';
        case 0x2F: return 'v'; case 0x11: return 'w'; case 0x2D: return 'x';
        case 0x15: return 'y'; case 0x2C: return 'z';
        case 0x02: return '1'; case 0x03: return '2'; case 0x04: return '3';
        case 0x05: return '4'; case 0x06: return '5'; case 0x07: return '6';
        case 0x08: return '7'; case 0x09: return '8'; case 0x0A: return '9';
        case 0x0B: return '0'; case 0x39: return ' '; case 0x1C: return '\n';
        case 0x0E: return '\b'; case 0x35: return '/'; case 0x0C: return '-';
        default: return 0;
    }
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

int strncmp(const char* s1, const char* s2, int n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int has_pending_request = 0;
int is_locked_down = 0;
char pending_ip[32] = "";

void kernel_main(void) {
    init_serial();
    init_gdt();

    print_serial("\033[2J\033[H");
    print_serial("=====================================================\r\n");
    print_serial("       MOKSHA MICROKERNEL v0.7 (KEYBOARD DRIVER CORE)\r\n");
    print_serial("=====================================================\r\n");
    print_serial("[PS/2 DRIVER] Keyboard Controller on Port 0x60: READY\r\n");
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
            print_serial("  key-test     - Listen & decode raw PS/2 hardware scancodes\r\n");
            print_serial("  admin-req    - Fast-track verified 10.0.0.1 request\r\n");
            print_serial("  unknown-req  - Ingest quarantine packet & alert MCD\r\n");
            print_serial("  approve      - Authorize quarantined IP\r\n");
            print_serial("  deny         - Neutralize intruder & lockdown gates\r\n");
            print_serial("  reset        - Lift emergency lockdown\r\n");
            print_serial("  reboot       - Trigger hardware reboot\r\n");
        } else if (strcmp(buffer, "key-test") == 0) {
            print_serial("[KEYBOARD DRIVER TEST] Press keys (Press ESC or Enter to exit)...\r\n");
            while (1) {
                if (inb(PS2_STATUS_PORT) & 1) {
                    uint8_t sc = inb(PS2_DATA_PORT);
                    if (sc < 0x80) { // Key Press
                        char ascii = scancode_to_ascii(sc);
                        if (sc == 0x01 || sc == 0x1C) { // ESC or Enter
                            print_serial("\r\n[KEYBOARD TEST COMPLETED]\r\n");
                            break;
                        }
                        if (ascii) {
                            print_serial("Key Pressed: ");
                            write_serial(ascii);
                            print_serial(" (Scancode: 0x");
                            write_serial("0123456789ABCDEF"[sc >> 4]);
                            write_serial("0123456789ABCDEF"[sc & 0x0F]);
                            print_serial(")\r\n");
                        }
                    }
                }
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
