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

/* Official Admin Master IP Definition */
#define MASTER_ADMIN_IP "10.0.0.1"

/* Sentinel Firewall State */
int pending_authorization = 0;
char pending_ip[20];
char pending_cmd[32];
int emergency_lockdown = 0;
unsigned int visitor_seed = 8491;

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
    print_serial("[WARN] Interrupt Trap Triggered!\n");
}

void init_idt(void) {
    idt_ptr.limit = sizeof(struct idt_entry_struct) * 256 - 1;
    idt_ptr.base  = (unsigned int)&idt_entries;

    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, (unsigned long)default_interrupt_handler, 0x08, 0x8E);
    }

    __asm__ __volatile__("lidt (%0)" : : "r" (&idt_ptr));
}

void handle_ip_request(const char *ip, const char *cmd) {
    if (emergency_lockdown) {
        print_serial("[REJECTED] System is in FULL EMERGENCY LOCKDOWN. No requests allowed.\n");
        return;
    }

    /* Condition 1: Official Admin IP -> Fast-Track VIP Priority */
    if (strcmp(ip, MASTER_ADMIN_IP) == 0) {
        print_serial("\n>>> [VIP FAST-TRACK AUTHENTICATED] <<<\n");
        print_serial("Source IP: ");
        print_serial(ip);
        print_serial(" (OFFICIAL MOKSHA MASTER)\n");
        print_serial("Status: VIP Priority Granted. Zero latency execution: [");
        print_serial(cmd);
        print_serial("] -> SUCCESS!\n");
        return;
    }

    /* Condition 2: Unknown IP -> Trigger Sentinel Notification */
    pending_authorization = 1;
    strcpy(pending_ip, ip);
    strcpy(pending_cmd, cmd);

    print_serial("\n=======================================================\n");
    print_serial("[SENTINEL ALERT -> NOTIFICATION TO MOKSHA MASTER PHONE]\n");
    print_serial("Unknown / Foreign IP Detected: ");
    print_serial(ip);
    print_serial("\nTarget Command: '");
    print_serial(cmd);
    print_serial("'\nState: HELD IN QUARANTINE.\n");
    print_serial("Moksha Decision Required:\n");
    print_serial("  Type 'approve' -> Issue Dynamic Visitor Token\n");
    print_serial("  Type 'deny'    -> Engage Counter-Attack & Emergency Lockdown\n");
    print_serial("=======================================================\n");
}

void shell_run(void) {
    char buffer[64];
    int idx = 0;

    print_serial("\nCommands: 'admin-req', 'unknown-req', 'approve', 'deny', 'status', 'reset', 'clear'\n");
    print_serial("moksha-sentinel> ");

    while (1) {
        char c = read_serial();

        if (c == '\r' || c == '\n') {
            if (idx == 0) {
                continue;
            }
            write_serial('\r');
            write_serial('\n');
            buffer[idx] = '\0';

            if (emergency_lockdown && strcmp(buffer, "reset") != 0) {
                print_serial("[CRITICAL] EMERGENCY LOCKDOWN ACTIVE. System Frozen. Type 'reset' to recover.\n");
            } else if (strcmp(buffer, "help") == 0) {
                print_serial("Available Commands:\n");
                print_serial("  admin-req    - Test Official Admin IP (10.0.0.1) Fast-Track VIP\n");
                print_serial("  unknown-req  - Test Unknown IP access attempt\n");
                print_serial("  approve      - Approve unknown request with Visitor Token\n");
                print_serial("  deny         - Reject unknown request & launch Counter-Attack\n");
                print_serial("  status       - Show security status\n");
                print_serial("  reset        - Master reset for emergency gates\n");
            } else if (strcmp(buffer, "admin-req") == 0) {
                handle_ip_request(MASTER_ADMIN_IP, "EXECUTE_SYSTEM_CONFIG");
            } else if (strcmp(buffer, "unknown-req") == 0) {
                handle_ip_request("192.168.1.189", "ACCESS_SENSITIVE_STORAGE");
            } else if (strcmp(buffer, "approve") == 0) {
                if (!pending_authorization) {
                    print_serial("[INFO] No pending requests to approve.\n");
                } else {
                    visitor_seed = (visitor_seed * 1103515245 + 12345) & 0x7FFFFFFF;
                    int visitor_id = 1000 + (visitor_seed % 9000);
                    pending_authorization = 0;
                    print_serial("\n[MOKSHA APPROVED] Temporary Visitor Token Generated!\n");
                    print_serial("-> Pass ID: [VISITOR-");
                    print_dec(visitor_id);
                    print_serial("] for IP: ");
                    print_serial(pending_ip);
                    print_serial("\n-> Gates: TEMPORARILY OPEN (Monitored & Logged)\n");
                }
            } else if (strcmp(buffer, "deny") == 0) {
                if (!pending_authorization) {
                    print_serial("[INFO] No pending requests to deny.\n");
                } else {
                    pending_authorization = 0;
                    emergency_lockdown = 1;
                    print_serial("\n*******************************************************\n");
                    print_serial("[MOKSHA REJECTED] UNKNOWN IP ATTACK DETECTED!\n");
                    print_serial("-> INITIATING AUTOMATIC DEFENSIVE COUNTER-STRIKE ON: ");
                    print_serial(pending_ip);
                    print_serial("\n-> IP ");
                    print_serial(pending_ip);
                    print_serial(" PERMANENTLY BLACKLISTED!\n");
                    print_serial("-> EMERGENCY GATES ENGAGED: FULL KERNEL LOCKDOWN APPLIED!\n");
                    print_serial("*******************************************************\n");
                }
            } else if (strcmp(buffer, "status") == 0) {
                print_serial("[Sentinel Dual-Tier Firewall Status]\n");
                print_serial("  Official Master IP: 10.0.0.1 (VIP Fast-Track)\n");
                print_serial("  Emergency Lockdown: ");
                print_serial(emergency_lockdown ? "ACTIVE (SYSTEM IS FROZEN)\n" : "NORMAL (SECURE)\n");
                print_serial("  Pending Alert: ");
                print_serial(pending_authorization ? "YES (Waiting for Moksha Decision)\n" : "NONE\n");
            } else if (strcmp(buffer, "reset") == 0) {
                emergency_lockdown = 0;
                pending_authorization = 0;
                print_serial("[MASTER OVERRIDE] Emergency lockdown lifted. Firewall reset to normal.\n");
            } else if (strcmp(buffer, "clear") == 0) {
                print_serial("\033[2J\033[H");
            } else {
                print_serial("Unknown command: ");
                print_serial(buffer);
                print_serial("\n");
            }

            idx = 0;
            print_serial("moksha-sentinel> ");
        } else if (c == 0x7F || c == '\b') {
            if (idx > 0) {
                idx--;
                print_serial("\b \b");
            }
        } else if (idx < 63 && c >= 32 && c <= 126) {
            buffer[idx++] = c;
            write_serial(c);
        }
    }
}

void kernel_main(void) {
    init_serial();
    print_serial("\n========================================\n");
    print_serial("[MOKSHA KERNEL] Booting v0.5 (Sentinel Dual-Tier Firewall)...\n");
    print_serial("[OK] Serial Driver Active.\n");

    init_gdt();
    print_serial("[OK] GDT Security Ring Active.\n");

    init_idt();
    print_serial("[OK] IDT Shield Loaded.\n");

    print_serial("[OK] Master IP (10.0.0.1) Fast-Track Online!\n");
    print_serial("[OK] 2FA Unknown IP Notification Shield Active!\n");
    print_serial("========================================\n");

    shell_run();
}
