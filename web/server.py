#!/usr/bin/env python3
import http.server
import json
import os
import socketserver
import subprocess
import sys

PORT = 8080
MAX_BODY_BYTES = 1024 * 1024
MAX_SOURCE_BYTES = 512 * 1024
MAX_ITERATIONS = 1000
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
COMPILER_BIN = os.path.join(PROJECT_ROOT, "compiler.exe")

class CompilerHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=SCRIPT_DIR, **kwargs)

    def do_POST(self):
        if self.path == "/api/compile":
            try:
                content_length = int(self.headers.get("Content-Length", 0))
                if content_length < 0 or content_length > MAX_BODY_BYTES:
                    self.send_error(413, "Request body is too large")
                    return
                post_body = self.rfile.read(content_length)
                req = json.loads(post_body.decode("utf-8"))
                if not isinstance(req, dict):
                    raise ValueError("JSON payload must be an object")
            except (ValueError, UnicodeDecodeError, json.JSONDecodeError) as e:
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
            if not isinstance(source, str) or len(source.encode("utf-8")) > MAX_SOURCE_BYTES:
                self.send_error(413, "Source is missing or too large")
                return
            if not all(isinstance(value, bool) for value in (enable_cf, enable_cp, enable_dce)):
                self.send_error(400, "Optimization flags must be boolean")
                return
            if (isinstance(max_iterations, bool) or
                    not isinstance(max_iterations, int) or
                    not 1 <= max_iterations <= MAX_ITERATIONS):
                self.send_error(400, "max_iterations must be an integer between 1 and 1000")
                return

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
                    cwd=PROJECT_ROOT,
                    timeout=5
                )
                output = proc.stdout
                if not output.strip() and proc.stderr:
                    output = json.dumps({"success": False, "errors": [{"message": proc.stderr, "phase": "runtime"}]})
                try:
                    json.loads(output)
                except json.JSONDecodeError:
                    self.send_error(502, "Compiler returned invalid JSON")
                    return

                self.send_response(200 if proc.returncode == 0 else 422)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(output.encode("utf-8"))
            except subprocess.TimeoutExpired:
                self.send_error(504, "Compilation timed out")
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
        self.send_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

def main():
    if not os.path.exists(COMPILER_BIN):
        print(f"Warning: {COMPILER_BIN} not found! Run build command first.")

    with socketserver.TCPServer(("127.0.0.1", PORT), CompilerHandler) as httpd:
        print(f"Serving Compiler Optimization Visualizer at http://localhost:{PORT}")
        print("Press Ctrl+C to stop.")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server.")

if __name__ == "__main__":
    main()
