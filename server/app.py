"""
Flask server for ESP8266 4WD Robot Chassis
Runs on a PC connected to the same phone hotspot as the robot.

Usage:
    pip install flask
    python app.py

Dashboard: http://localhost:5000
"""

from flask import Flask, render_template, request, jsonify
from collections import deque
import time

app = Flask(__name__)

# --- Debug log buffer (last 200 messages) ---
debug_logs = deque(maxlen=200)

# --- Robot state (updated by ESP8266 POST) ---
robot_state = {
    "front": 0, "left": 0, "right": 0,
    "heading": 0,
    "pos_x": 0, "pos_y": 0,
    "distance": 0, "enc_ticks": 0,
    "battery": 0,
    "state": 0, "state_name": "IDLE",
    "path_length": 0, "auto": True,
    "grid_x": 10, "grid_y": 10,
    "target_x": 10, "target_y": 10,
    "target_wx": 0, "target_wy": 0,
    "has_target": False,
    "target_reached": False,
    "backtracking": False,
    "crumbs": 0,
    "connected": False,
    "last_update": 0,
    "debug_mode": False,
    "wifi_rssi": 0,
    "free_heap": 0,
    "uptime": 0,
}

# --- Pending command (consumed by ESP8266 GET) ---
pending_command = {"cmd": "none"}

STATE_NAMES = {
    0: "IDLE",
    1: "SLOWDOWN",
    2: "BRAKE",
    3: "SCANNING",
    4: "REVERSING",
    5: "TURNING",
}

GRID_SIZE = 20


@app.route("/")
def dashboard():
    return render_template("dashboard.html", grid_size=GRID_SIZE)


# ============================================
# ESP8266 -> Server
# ============================================

@app.route("/api/robot/report", methods=["POST"])
def robot_report():
    global robot_state
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "no json"}), 400

    robot_state.update({
        "front": data.get("front", 0),
        "left": data.get("left", 0),
        "right": data.get("right", 0),
        "heading": data.get("heading", 0),
        "pos_x": data.get("pos_x", 0),
        "pos_y": data.get("pos_y", 0),
        "distance": data.get("distance", 0),
        "enc_ticks": data.get("enc_ticks", 0),
        "battery": data.get("battery", 0),
        "state": data.get("state", 0),
        "state_name": STATE_NAMES.get(data.get("state", 0), "UNKNOWN"),
        "path_length": data.get("path_length", 0),
        "auto": data.get("auto", True),
        "grid_x": data.get("grid_x", 10),
        "grid_y": data.get("grid_y", 10),
        "target_x": data.get("target_x", 10),
        "target_y": data.get("target_y", 10),
        "target_wx": data.get("target_wx", 0),
        "target_wy": data.get("target_wy", 0),
        "has_target": data.get("has_target", False),
        "target_reached": data.get("target_reached", False),
        "backtracking": data.get("backtracking", False),
        "crumbs": data.get("crumbs", 0),
        "debug_mode": data.get("debug_mode", False),
        "wifi_rssi": data.get("wifi_rssi", 0),
        "free_heap": data.get("free_heap", 0),
        "uptime": data.get("uptime", 0),
        "connected": True,
        "last_update": time.time(),
    })
    return jsonify({"ok": True})


@app.route("/api/robot/command", methods=["GET"])
def robot_command():
    global pending_command
    cmd = pending_command.copy()
    pending_command = {"cmd": "none"}
    return jsonify(cmd)


# ============================================
# Dashboard -> Server
# ============================================

@app.route("/api/status", methods=["GET"])
def get_status():
    state = robot_state.copy()
    if time.time() - state["last_update"] > 3:
        state["connected"] = False
    return jsonify(state)


@app.route("/api/control", methods=["POST"])
def send_control():
    global pending_command
    data = request.get_json(silent=True)
    if not data or "cmd" not in data:
        return jsonify({"error": "missing cmd"}), 400

    cmd = data["cmd"]
    valid = ("forward", "back", "left", "right", "stop", "auto",
             "backtrack", "reset",
             "test_all", "test_i2c", "test_ultrasonic", "test_servo",
             "test_mpu", "test_motors", "test_encoder",
             "test_battery")
    if cmd not in valid:
        return jsonify({"error": "invalid cmd"}), 400

    pending_command = {"cmd": cmd}
    return jsonify({"ok": True, "cmd": cmd})


@app.route("/api/target", methods=["POST"])
def set_target():
    """Set target in real-world cm (relative to robot start position)."""
    global pending_command, robot_state
    data = request.get_json(silent=True)
    if not data:
        return jsonify({"error": "no json"}), 400

    x = data.get("x")
    y = data.get("y")
    if x is None or y is None:
        return jsonify({"error": "missing x or y"}), 400

    x = float(x)
    y = float(y)

    pending_command = {"cmd": "set_target", "x": x, "y": y}
    robot_state["target_wx"] = x
    robot_state["target_wy"] = y
    return jsonify({"ok": True, "x": x, "y": y})


# ============================================
# Debug endpoints
# ============================================

@app.route("/api/robot/debug", methods=["POST"])
def robot_debug():
    """ESP8266 sends debug log messages here."""
    data = request.get_json(silent=True)
    if not data or "logs" not in data:
        return jsonify({"error": "no logs"}), 400

    ts = time.strftime("%H:%M:%S")
    for msg in data["logs"]:
        debug_logs.append(f"[{ts}] {msg}")
        print(f"  [ROBOT] {msg}")  # Also print to server console

    return jsonify({"ok": True})


@app.route("/api/debug/logs", methods=["GET"])
def get_debug_logs():
    """Dashboard polls this for debug log messages."""
    since = int(request.args.get("since", 0))
    logs = list(debug_logs)
    # Return logs newer than 'since' index
    if since < len(logs):
        return jsonify({"logs": logs[since:], "total": len(logs)})
    return jsonify({"logs": [], "total": len(logs)})


@app.route("/api/debug/clear", methods=["POST"])
def clear_debug_logs():
    """Clear all debug logs."""
    debug_logs.clear()
    return jsonify({"ok": True})


if __name__ == "__main__":
    print("=" * 50)
    print("  4WD Robot Server")
    print("  Connect your PC to the phone hotspot first!")
    print("  Dashboard: http://localhost:5000")
    print("=" * 50)
    app.run(host="0.0.0.0", port=5000, debug=True)
