"""
ESP32-S3 4WD Robot Server

Can run locally (LAN) or deployed to cloud (internet).
API token prevents unauthorized access when on the internet.

Local:  python app.py
Cloud:  gunicorn app:app (Render/Railway auto-detect this)

Dashboard: http://localhost:5000  (or your cloud URL)
"""

import os
from flask import Flask, render_template, request, jsonify
from collections import deque
from functools import wraps
import time

app = Flask(__name__)

# --- API Token ---
# Set via environment variable for cloud, or use default for local dev.
# ESP32 and dashboard must send this token in headers.
# To generate a random token: python -c "import secrets; print(secrets.token_hex(16))"
API_TOKEN = os.environ.get("ROBOT_API_TOKEN", "robot123")

# --- Debug log buffer ---
debug_logs = deque(maxlen=200)

# --- Robot state ---
robot_state = {
    "front": 0, "left": 0, "right": 0,
    "heading": 0, "compass_heading": -1, "gyro_rate": 0,
    "pos_x": 0, "pos_y": 0,
    "distance": 0, "enc_l": 0, "enc_r": 0,
    "battery": 0,
    "state": 0, "state_name": "IDLE",
    "path_length": 0, "auto": True,
    "grid_x": 20, "grid_y": 20,
    "target_x": 20, "target_y": 20,
    "target_wx": 0, "target_wy": 0,
    "has_target": False,
    "target_reached": False,
    "backtracking": False,
    "crumbs": 0,
    "compass_ok": False, "mpu_ok": False,
    "connected": False,
    "last_update": 0,
    "debug_mode": False,
    "wifi_rssi": 0,
    "free_heap": 0,
    "uptime": 0,
}

pending_command = {"cmd": "none"}

STATE_NAMES = {
    0: "IDLE", 1: "SLOWDOWN", 2: "BRAKE",
    3: "SCANNING", 4: "REVERSING", 5: "TURNING",
}

GRID_SIZE = 40


# ============================================
# Auth: token check for API endpoints
# ============================================
def require_token(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        token = request.headers.get("X-Robot-Token") or request.args.get("token")
        if token != API_TOKEN:
            return jsonify({"error": "unauthorized"}), 401
        return f(*args, **kwargs)
    return decorated


# ============================================
# Dashboard (serves HTML, token passed via template)
# ============================================
@app.route("/")
def dashboard():
    return render_template("dashboard.html", grid_size=GRID_SIZE, api_token=API_TOKEN)


# ============================================
# Robot -> Server (ESP32 endpoints)
# ============================================
@app.route("/api/robot/report", methods=["POST"])
@require_token
def robot_report():
    global robot_state
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "no json"}), 400

    robot_state.update({k: data.get(k, robot_state.get(k, 0)) for k in data})
    robot_state["state_name"] = STATE_NAMES.get(data.get("state", 0), "UNKNOWN")
    robot_state["connected"] = True
    robot_state["last_update"] = time.time()
    return jsonify({"ok": True})


@app.route("/api/robot/command", methods=["GET"])
@require_token
def robot_command():
    global pending_command
    cmd = pending_command.copy()
    pending_command = {"cmd": "none"}
    return jsonify(cmd)


@app.route("/api/robot/debug", methods=["POST"])
@require_token
def robot_debug():
    data = request.get_json(silent=True)
    if not data or "logs" not in data:
        return jsonify({"error": "no logs"}), 400

    ts = time.strftime("%H:%M:%S")
    for msg in data["logs"]:
        debug_logs.append(f"[{ts}] {msg}")
        print(f"  [ROBOT] {msg}")

    return jsonify({"ok": True})


# ============================================
# Dashboard -> Server (browser endpoints)
# ============================================
@app.route("/api/status", methods=["GET"])
@require_token
def get_status():
    state = robot_state.copy()
    if time.time() - state["last_update"] > 3:
        state["connected"] = False
    return jsonify(state)


@app.route("/api/control", methods=["POST"])
@require_token
def send_control():
    global pending_command
    data = request.get_json(silent=True)
    if not data or "cmd" not in data:
        return jsonify({"error": "missing cmd"}), 400

    cmd = data["cmd"]
    valid = ("forward", "back", "left", "right", "stop", "auto",
             "backtrack", "reset",
             "test_all", "test_i2c", "test_ultrasonic", "test_servo",
             "test_mpu", "test_compass", "test_motors", "test_encoder",
             "test_battery")
    if cmd not in valid:
        return jsonify({"error": "invalid cmd"}), 400

    pending_command = {"cmd": cmd}
    return jsonify({"ok": True, "cmd": cmd})


@app.route("/api/target", methods=["POST"])
@require_token
def set_target():
    global pending_command, robot_state
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "no json"}), 400

    x = data.get("x")
    y = data.get("y")
    if x is None or y is None:
        return jsonify({"error": "missing x or y"}), 400

    pending_command = {"cmd": "set_target", "x": float(x), "y": float(y)}
    robot_state["target_wx"] = float(x)
    robot_state["target_wy"] = float(y)
    return jsonify({"ok": True, "x": x, "y": y})


@app.route("/api/debug/logs", methods=["GET"])
@require_token
def get_debug_logs():
    since = int(request.args.get("since", 0))
    logs = list(debug_logs)
    if since < len(logs):
        return jsonify({"logs": logs[since:], "total": len(logs)})
    return jsonify({"logs": [], "total": len(logs)})


@app.route("/api/debug/clear", methods=["POST"])
@require_token
def clear_debug_logs():
    debug_logs.clear()
    return jsonify({"ok": True})


# ============================================
# Health check (for cloud platforms)
# ============================================
@app.route("/health")
def health():
    return jsonify({"status": "ok", "connected": robot_state["connected"]})


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 5000))
    print("=" * 50)
    print("  4WD Robot Server")
    print(f"  Token: {API_TOKEN}")
    print(f"  Dashboard: http://localhost:{port}")
    print("=" * 50)
    app.run(host="0.0.0.0", port=port, debug=True)
