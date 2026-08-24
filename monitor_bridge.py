import urllib.request
import urllib.parse
import json
import subprocess
import threading
import time
import sys

BOT_TOKEN = "8178324920:AAE2wBcWYCEufF6sWJLi6iZLfOWZ3wG1O6s"
CHAT_ID = "8657781941"

kernel_process = None
last_update_id = 0

def send_tg(msg):
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
    payload = {
        "chat_id": CHAT_ID,
        "text": f"🛡️ *[MOKSHA SENTINEL 2-WAY CONTROL]*\n\n{msg}",
        "parse_mode": "Markdown"
    }
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
    try:
        urllib.request.urlopen(req, timeout=5)
    except Exception:
        pass

def telegram_poller():
    global last_update_id
    while True:
        try:
            url = f"https://api.telegram.org/bot{BOT_TOKEN}/getUpdates?offset={last_update_id + 1}&timeout=30"
            req = urllib.request.Request(url)
            response = urllib.request.urlopen(req, timeout=35)
            data = json.loads(response.read().decode('utf-8'))
            
            if data.get("ok"):
                for result in data.get("result", []):
                    last_update_id = result["update_id"]
                    message = result.get("message", {})
                    text = message.get("text", "").strip()
                    sender_chat = str(message.get("chat", {}).get("id", ""))
                    
                    if sender_chat == CHAT_ID and kernel_process and kernel_process.poll() is None:
                        if text == "/approve":
                            kernel_process.stdin.write("approve\n")
                            kernel_process.stdin.flush()
                            send_tg("📥 *Command Received from Telegram:* `/approve`\nForwarded to Moksha Kernel Core.")
                        elif text == "/deny":
                            kernel_process.stdin.write("deny\n")
                            kernel_process.stdin.flush()
                            send_tg("📥 *Command Received from Telegram:* `/deny`\nForwarded to Moksha Kernel Core.")
        except Exception:
            time.sleep(2)

def start_kernel():
    global kernel_process
    kernel_process = subprocess.Popen(
        ["qemu-system-x86_64", "-kernel", "mykernel.bin", "-display", "none", "-serial", "stdio"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )

    send_tg("🚀 *Moksha Microkernel v0.5 Booted!*\n\n- 2-Way Telegram Polling: `ACTIVE`\n- Master IP: `10.0.0.1`\n- Ready for remote `/approve` or `/deny` commands.")

    # Start Telegram background polling thread
    t = threading.Thread(target=telegram_poller, daemon=True)
    t.start()

    for line in kernel_process.stdout:
        sys.stdout.write(line)
        sys.stdout.flush()

        if "[SENTINEL ALERT -> NOTIFICATION" in line:
            send_tg("⚠️ *UNAUTHORIZED IP DETECTED!*\n\nForeign IP attempt held in quarantine.\n👉 Send `/approve` or `/deny` directly in chat to control gates.")
        elif "[MOKSHA APPROVED]" in line:
            send_tg("✅ *ACCESS GRANTED*\n\nTemporary Visitor Pass unlocked via Telegram command.")
        elif "[MOKSHA REJECTED]" in line:
            send_tg("🚨 *CRITICAL SECURITY STRIKE!*\n\nIntruder Counter-Attacked and Blacklisted!\n*EMERGENCY GATES FULL LOCKDOWN ENGAGED!*")
        elif "[MASTER OVERRIDE]" in line:
            send_tg("🔓 *MASTER RESET*\n\nEmergency lockdown lifted.")

if __name__ == "__main__":
    start_kernel()
