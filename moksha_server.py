import os
import sys
import subprocess
from http.server import HTTPServer, BaseHTTPRequestHandler
import json
import urllib.request

PORT = 8080
TOPIC = "Moksha-Master-Core"

def push_ntfy(title, msg):
    try:
        req = urllib.request.Request(
            f"https://ntfy.sh/{TOPIC}",
            data=msg.encode("utf-8"),
            headers={"Title": title, "Priority": "high", "Tags": "server,shield"}
        )
        urllib.request.urlopen(req, timeout=3)
    except Exception:
        pass

class MokshaServerHandler(BaseHTTPRequestHandler):
    def _set_headers(self, status=200):
        self.send_response(status)
        self.send_header('Content-type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()

    def do_GET(self):
        if self.path == "/status":
            self._set_headers(200)
            res = {
                "server": "Moksha Microkernel Dedicated Server v1.0",
                "core_status": "ONLINE",
                "arch": "x86_32 Protected Mode",
                "host": "Termux Core Node"
            }
            self.wfile.write(json.dumps(res, indent=2).encode('utf-8'))
        elif self.path == "/boot":
            self._set_headers(200)
            push_ntfy("🚀 Server Remote Boot", "Moksha Kernel boot trigger received from Server Node.")
            subprocess.Popen(["python3", "monitor_bridge.py"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            res = {"status": "SUCCESS", "message": "Kernel boot sequence triggered via Bridge."}
            self.wfile.write(json.dumps(res).encode('utf-8'))
        else:
            self._set_headers(404)
            self.wfile.write(json.dumps({"error": "Route Not Found"}).encode('utf-8'))

    def log_message(self, format, *args):
        print(f"[MOKSHA SERVER LOG] {self.address_string()} - {args[0]}")

def start_server():
    server_address = ('127.0.0.1', PORT)
    httpd = HTTPServer(server_address, MokshaServerHandler)
    print(f"🛡️ MOKSHA DEDICATED SERVER RUNNING ON PORT {PORT}")
    push_ntfy("⚡ Dedicated Server Active", f"Termux Core Server listening on port {PORT}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        httpd.server_close()

if __name__ == "__main__":
    start_server()
