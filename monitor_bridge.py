import urllib.request
import urllib.parse
import json
import subprocess
import sys

BOT_TOKEN = "8178324920:AAE2wBcWYCEufF6sWJLi6iZLfOWZ3wG1O6s"
CHAT_ID = "8657781941"

def send_tg(msg):
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
    payload = {
        "chat_id": CHAT_ID,
        "text": f"🛡️ *[MOKSHA SENTINEL SECURITY ALERT]*\n\n{msg}",
        "parse_mode": "Markdown"
    }
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
    try:
        urllib.request.urlopen(req, timeout=5)
    except Exception:
        pass

def listen_and_relay():
    proc = subprocess.Popen(
        ["qemu-system-x86_64", "-kernel", "mykernel.bin", "-display", "none", "-serial", "stdio"],
        stdin=sys.stdin,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    send_tg("🚀 *Moksha Microkernel Booted!*\n\n- VIP Master IP: `10.0.0.1` Active\n- Sentinel Firewall: Online\n- Live Monitoring: Enabled")

    for line in proc.stdout:
        sys.stdout.write(line)
        sys.stdout.flush()

        if "[SENTINEL ALERT -> NOTIFICATION" in line:
            send_tg("⚠️ *UNAUTHORIZED IP DETECTED!*\n\nForeign IP attempt blocked in quarantine. Action required in terminal: `approve` or `deny`.")
        elif "[MOKSHA APPROVED]" in line:
            send_tg("✅ *ACCESS GRANTED BY MASTER*\n\nTemporary Visitor Token issued. Security Gates: Temporary Pass Active.")
        elif "[MOKSHA REJECTED]" in line:
            send_tg("🚨 *CRITICAL SECURITY STRIKE!*\n\nUnauthorized IP has been Counter-Attacked and Blacklisted!\n*EMERGENCY GATES: FULL LOCKDOWN APPLIED!*")
        elif "[MASTER OVERRIDE]" in line:
            send_tg("🔓 *MASTER RESET*\n\nEmergency lockdown lifted by Master Authority.")

if __name__ == "__main__":
    listen_and_relay()
