import os
import sys
import pty
import time
import json
import urllib.request
import urllib.parse
import threading

TOKEN = "8125867169:AAGbS45D6z67VzK-6kY2YJ0uH_y2j_bL7zM"
CHAT_ID = "6167884107"

def send_telegram(message):
    try:
        url = f"https://api.telegram.org/bot{TOKEN}/sendMessage"
        payload = {"chat_id": CHAT_ID, "text": message}
        data = json.dumps(payload).encode("utf-8")
        req = urllib.request.Request(url, data=data, headers={"Content-Type": "application/json"})
        urllib.request.urlopen(req, timeout=5)
    except Exception:
        pass

def poll_telegram_commands(master_fd):
    last_update_id = 0
    url = f"https://api.telegram.org/bot{TOKEN}/getUpdates"
    
    while True:
        try:
            req_url = f"{url}?offset={last_update_id + 1}&timeout=2"
            with urllib.request.urlopen(req_url, timeout=5) as response:
                data = json.loads(response.read().decode('utf-8'))
                if data.get("ok"):
                    for result in data.get("result", []):
                        last_update_id = result["update_id"]
                        msg = result.get("message", {}).get("text", "").strip()
                        
                        if msg == "/approve":
                            send_telegram("🛡️ [MOKSHA CONTROL' DECK]\n\n📥 MCD Action: /approve dispatched to Kernel.")
                            os.write(master_fd, b"approve\n")
                        elif msg == "/deny":
                            send_telegram("🛡️ [MOKSHA CONTROL' DECK]\n\n🚨 MCD Action: /deny dispatched! Lockdown initiated.")
                            os.write(master_fd, b"deny\n")
                        elif msg == "/reset":
                            send_telegram("🛡️ [MOKSHA CONTROL' DECK]\n\n🔓 MCD Action: /reset dispatched! Lifting emergency lockdown...")
                            os.write(master_fd, b"reset\n")
                        elif msg == "/status":
                            send_telegram("🛡️ [MOKSHA CONTROL' DECK]\n\n📊 MCD Action: Requesting AI status scan...")
                            os.write(master_fd, b"ai-status\n")
        except Exception:
            pass
        time.sleep(1)

def run_qemu_with_bridge():
    master_fd, slave_fd = pty.openpty()
    
    t = threading.Thread(target=poll_telegram_commands, args=(master_fd,), daemon=True)
    t.start()
    
    send_telegram("🚀 Moksha Microkernel v0.8 Online!\n\n- Master VIP Route: 10.0.0.1\n- Remote MCD Deck: Active (/approve, /deny, /reset, /status)")

    pid = os.fork()
    if pid == 0:
        os.close(master_fd)
        os.dup2(slave_fd, 0)
        os.dup2(slave_fd, 1)
        os.dup2(slave_fd, 2)
        os.close(slave_fd)
        os.execlp("qemu-system-x86_64", "qemu-system-x86_64", "-kernel", "mykernel.bin", "-serial", "stdio", "-display", "none")
    else:
        os.close(slave_fd)
        while True:
            try:
                data = os.read(master_fd, 1024)
                if not data:
                    break
                text = data.decode('utf-8', errors='ignore')
                sys.stdout.write(text)
                sys.stdout.flush()
                
                if "⚠️ [SENTINEL ALERT" in text:
                    send_telegram("🚨 [SENTINEL THREAT ALERT]\n\nSuspicious IP quarantined! Use /approve or /deny from mobile.")
                elif "INTRUDER BLACKLISTED" in text:
                    send_telegram("🚨 [EMERGENCY LOCKDOWN]\n\nIntruder neutralized & gates locked! Use /reset to restore.")
                elif "Lockdown Lifted" in text:
                    send_telegram("🔓 [SYSTEM NORMALIZED]\n\nLockdown lifted. Master authority confirmed.")
            except Exception:
                break

if __name__ == "__main__":
    run_qemu_with_bridge()
