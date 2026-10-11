#!/usr/bin/env python3
import http.server
import json
import os
import socketserver
import subprocess
import sys

PORT = 8080
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
COMPILER_BIN = os.path.join(PROJECT_ROOT, "compiler.exe")

class CompilerHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=SCRIPT_DIR, **kwargs)

    def do_POST(self):
        if self.path == "/api/compile":
            content_length = int(self.headers.get("Content-Length", 0))
            post_body = self.rfile.read(content_length)

            try:
                req = json.loads(post_body.decode("utf-8"))
            except Exception as e:
                self.send_response(400)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": f"Invalid JSON payload: {str(e)}"}).encode("utf-8"))
                return

            source = req.get("source", "")
            enable_cf = req.get("enable_cf", True)
            enable_cp = req.get("enable_cp", True)
            enable_dce = req.get("enable_dce", True)
            max_iterations = req.get("max_iterations", 10)

            args = [COMPILER_BIN]
            if not enable_cf: args.append("--no-cf")
            if not enable_cp: args.append("--no-cp")
            if not enable_dce: args.append("--no-dce")
            args.extend(["--max-iterations", str(max_iterations)])

            try:
                proc = subprocess.run(
                    args,
                    input=source,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    cwd=PROJECT_ROOT
                )
                output = proc.stdout
                if not output.strip() and proc.stderr:
                    output = json.dumps({"success": False, "errors": [{"message": proc.stderr, "phase": "runtime"}]})

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(output.encode("utf-8"))
            except Exception as e:
                self.send_response(500)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": str(e)}).encode("utf-8"))
        else:
            self.send_response(404)
            self.end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

def main():
    if not os.path.exists(COMPILER_BIN):
        print(f"Warning: {COMPILER_BIN} not found! Run build command first.")

    with socketserver.TCPServer(("", PORT), CompilerHandler) as httpd:
        print(f"Serving Compiler Optimization Visualizer at http://localhost:{PORT}")
        print("Press Ctrl+C to stop.")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")

if __name__ == "__main__":
    main()
