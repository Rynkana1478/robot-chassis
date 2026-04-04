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
from ai_translator import translate as ai_translate, check_ollama

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
# AI Command Translator
# ============================================
ai_command_queue = deque()  # Multi-step commands from AI
ai_log = deque(maxlen=50)   # AI decision history

@app.route("/api/ai/translate", methods=["POST"])
@require_token
def ai_translate_endpoint():
    """Translate natural language → robot commands via Ollama/rules."""
    global pending_command
    data = request.get_json(silent=True)
    if not data or "text" not in data:
        return jsonify({"error": "missing text"}), 400

    user_text = data["text"].strip()
    if not user_text:
        return jsonify({"error": "empty text"}), 400

    # Translate
    result = ai_translate(user_text)

    # Log to AI history
    ai_log.append(result)

    # Log to debug console
    ts = time.strftime("%H:%M:%S")
    debug_logs.append(f"[{ts}] [AI] Input: \"{user_text}\"")
    debug_logs.append(f"[{ts}] [AI] Method: {result['method']} ({result['duration_ms']}ms)")
    print(f"  [AI] Input: \"{user_text}\" → Method: {result['method']} ({result['duration_ms']}ms)")

    if result.get("error"):
        debug_logs.append(f"[{ts}] [AI] ERROR: {result['error']}")
        print(f"  [AI] ERROR: {result['error']}")
        return jsonify(result)

    debug_logs.append(f"[{ts}] [AI] Explanation: {result['explanation']}")
    print(f"  [AI] Explanation: {result['explanation']}")

    # Queue commands
    commands = result.get("commands", [])
    for i, cmd in enumerate(commands):
        action = cmd.get("action", "")
        debug_logs.append(f"[{ts}] [AI] Command [{i+1}/{len(commands)}]: {json.dumps(cmd)}")
        print(f"  [AI] Command [{i+1}/{len(commands)}]: {cmd}")

    # Execute first command immediately, queue the rest
    if commands:
        first = commands[0]
        _execute_ai_command(first)

        for cmd in commands[1:]:
            ai_command_queue.append(cmd)

        if len(commands) > 1:
            debug_logs.append(f"[{ts}] [AI] Queued {len(commands)-1} more command(s)")

    return jsonify(result)


@app.route("/api/ai/status", methods=["GET"])
@require_token
def ai_status():
    """Get AI system status and recent history."""
    return jsonify({
        "ollama_available": check_ollama(),
        "queue_length": len(ai_command_queue),
        "history": list(ai_log),
    })


@app.route("/api/ai/next", methods=["POST"])
@require_token
def ai_next_command():
    """Execute next queued AI command (called when robot reaches target)."""
    if ai_command_queue:
        cmd = ai_command_queue.popleft()
        _execute_ai_command(cmd)
        ts = time.strftime("%H:%M:%S")
        debug_logs.append(f"[{ts}] [AI] Next queued command: {json.dumps(cmd)}")
        return jsonify({"ok": True, "cmd": cmd, "remaining": len(ai_command_queue)})
    return jsonify({"ok": True, "cmd": None, "remaining": 0})


import json as json_module

def _execute_ai_command(cmd):
    """Convert an AI command dict to a pending robot command."""
    global pending_command
    action = cmd.get("action", "")

    if action == "set_target":
        x = cmd.get("x", 0)
        y = cmd.get("y", 0)
        pending_command = {"cmd": "set_target", "x": float(x), "y": float(y)}
        robot_state["target_wx"] = float(x)
        robot_state["target_wy"] = float(y)
    elif action == "backtrack":
        pending_command = {"cmd": "backtrack"}
    elif action == "stop":
        pending_command = {"cmd": "stop"}
    elif action == "reset":
        pending_command = {"cmd": "reset"}


# ============================================
# Health check (for cloud platforms)
# ============================================
@app.route("/health")
def health():
    return jsonify({"status": "ok", "connected": robot_state["connected"]})


if __name__ == "__main__":
    port = int(os.environ.get("PORT", 25565))
    print("=" * 50)
    print("  4WD Robot Server")
    print(f"  Token: {API_TOKEN}")
    print(f"  Dashboard: http://localhost:{port}")
    print("=" * 50)
    app.run(host="0.0.0.0", port=port, debug=True)
