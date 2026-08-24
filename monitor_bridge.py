import urllib.request
import urllib.parse
import json
import subprocess
import threading
import time
import os
import pty
import select
import sys

BOT_TOKEN = "8178324920:AAE2wBcWYCEufF6sWJLi6iZLfOWZ3wG1O6s"
CHAT_ID = "8657781941"

master_fd = None
last_update_id = 0

def send_tg(msg):
    url = f"https://api.telegram.org/bot{BOT_TOKEN}/sendMessage"
    payload = {
        "chat_id": CHAT_ID,
        "text": f"🛡️ *[MOKSHA CONTROL' DECK]*\n\n{msg}",
        "parse_mode": "Markdown"
    }
    data = json.dumps(payload).encode('utf-8')
    req = urllib.request.Request(url, data=data, headers={'Content-Type': 'application/json'})
    try:
        urllib.request.urlopen(req, timeout=5)
    except Exception:
        pass

def telegram_poller():
    global last_update_id, master_fd
    while True:
        try:
            url = f"https://api.telegram.org/bot{BOT_TOKEN}/getUpdates?offset={last_update_id + 1}&timeout=10"
            req = urllib.request.Request(url)
            response = urllib.request.urlopen(req, timeout=15)
            data = json.loads(response.read().decode('utf-8'))
            
            if data.get("ok"):
                for result in data.get("result", []):
                    last_update_id = result["update_id"]
                    message = result.get("message", {})
                    text = message.get("text", "").strip()
                    sender_chat = str(message.get("chat", {}).get("id", ""))
                    
                    if sender_chat == CHAT_ID and master_fd is not None:
                        if text == "/approve":
                            os.write(master_fd, b"approve\r\n")
                            send_tg("📥 *MCD Action Executed:* `/approve` forwarded to Kernel Core.")
                        elif text == "/deny":
                            os.write(master_fd, b"deny\r\n")
                            send_tg("📥 *MCD Action Executed:* `/deny` forwarded to Kernel Core.")
        except Exception:
            time.sleep(1)

def run_system():
    global master_fd
    
    send_tg("🚀 *Moksha Microkernel Booted!*\n\n- Master IP: `10.0.0.1` (VIP Route)\n- MCD 2-Way Command Channel: `ONLINE`\n- Ready for real-time monitoring.")

    master, slave = pty.openpty()
    master_fd = master

    proc = subprocess.Popen(
        ["qemu-system-x86_64", "-kernel", "mykernel.bin", "-display", "none", "-serial", "stdio"],
        stdin=slave,
        stdout=slave,
        stderr=slave,
        close_fds=True
    )
    os.close(slave)

    # Start Telegram background thread
    t = threading.Thread(target=telegram_poller, daemon=True)
    t.start()

    line_buffer = ""
    
    # Non-blocking IO loop between Terminal & QEMU
    try:
        while proc.poll() is None:
            r, _, _ = select.select([master, sys.stdin.fileno()], [], [], 0.05)
            
            if master in r:
                data = os.read(master, 1024).decode('utf-8', errors='ignore')
                if not data:
                    break
                sys.stdout.write(data)
                sys.stdout.flush()
                
                line_buffer += data
                if "\n" in line_buffer or "\r" in line_buffer:
                    if "[SENTINEL ALERT -> NOTIFICATION" in line_buffer:
                        send_tg("⚠️ *UNAUTHORIZED IP DETECTED!*\n\nForeign IP attempt held in quarantine.\n👉 Send `/approve` or `/deny` from MCD to control gates.")
                    elif "[MOKSHA APPROVED]" in line_buffer:
                        send_tg("✅ *ACCESS GRANTED*\n\nTemporary Visitor Pass unlocked by Master.")
                    elif "[MOKSHA REJECTED]" in line_buffer:
                        send_tg("🚨 *CRITICAL SECURITY STRIKE!*\n\nIntruder Counter-Attacked and Blacklisted!\n*EMERGENCY GATES FULL LOCKDOWN ENGAGED!*")
                    elif "[MASTER OVERRIDE]" in line_buffer:
                        send_tg("🔓 *MASTER RESET*\n\nEmergency lockdown lifted.")
                    line_buffer = ""

            if sys.stdin.fileno() in r:
                user_in = os.read(sys.stdin.fileno(), 1024)
                os.write(master, user_in)
    except Exception:
        pass
    finally:
        os.close(master)
        proc.kill()

if __name__ == "__main__":
    run_system()
