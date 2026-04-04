"""
AI Command Translator
Converts natural language → robot commands using Ollama (local LLM).
Falls back to rule-based parsing if Ollama is not running.
"""

import json
import math
import re
import requests
import time

OLLAMA_URL = "http://localhost:11434/api/generate"
OLLAMA_MODEL = "llama3.2"

SYSTEM_PROMPT = """You translate human instructions into JSON robot commands. Output ONLY valid JSON.

Coordinate system: Forward=+Y, Backward=-Y, Left=-X, Right=+X. Units: centimeters. 1 meter = 100cm.

Cardinal directions (full distance on one axis):
- North/Forward: x=0, y=+distance
- South/Backward: x=0, y=-distance
- East/Right: x=+distance, y=0
- West/Left: x=-distance, y=0

Diagonal directions (multiply distance by 0.707 for BOTH x and y):
- Northeast: x=+distance*0.707, y=+distance*0.707
- Northwest: x=-distance*0.707, y=+distance*0.707
- Southeast: x=+distance*0.707, y=-distance*0.707
- Southwest: x=-distance*0.707, y=-distance*0.707

ONLY these actions exist. DO NOT invent new ones:
- "set_target" with "x" and "y" in cm (integers, round to whole numbers)
- "backtrack" (no params - return to start)
- "stop" (no params)
- "reset" (no params)

Output format: {"commands": [...], "explanation": "short text"}

Examples:
Input: "go forward 1 meter"
Output: {"commands": [{"action": "set_target", "x": 0, "y": 100}], "explanation": "Forward 100cm"}

Input: "go left 50cm"
Output: {"commands": [{"action": "set_target", "x": -50, "y": 0}], "explanation": "Left 50cm"}

Input: "come back"
Output: {"commands": [{"action": "backtrack"}], "explanation": "Returning to start"}

Input: "go right 2m then forward 1m"
Output: {"commands": [{"action": "set_target", "x": 200, "y": 0}, {"action": "set_target", "x": 200, "y": 100}], "explanation": "Right 200cm then forward 100cm"}

Input: "stop"
Output: {"commands": [{"action": "stop"}], "explanation": "Stop"}

Input: "go southeast 2 meters"
Output: {"commands": [{"action": "set_target", "x": 141, "y": -141}], "explanation": "Southeast 200cm diagonal (141,141)"}

Input: "go northwest 3 meters"
Output: {"commands": [{"action": "set_target", "x": -212, "y": 212}], "explanation": "Northwest 300cm diagonal (212,212)"}

Input: "go northeast 1m then come back"
Output: {"commands": [{"action": "set_target", "x": 71, "y": 71}, {"action": "backtrack"}], "explanation": "Northeast 100cm then return"}
"""


def check_ollama():
    """Check if Ollama is running."""
    try:
        r = requests.get("http://localhost:11434/api/tags", timeout=0.5)
        return r.status_code == 200
    except:
        return False


def ask_ollama(user_input):
    """Send text to Ollama, get JSON response."""
    try:
        r = requests.post(OLLAMA_URL, json={
            "model": OLLAMA_MODEL,
            "prompt": user_input,
            "system": SYSTEM_PROMPT,
            "stream": False,
            "format": "json",
            "options": {"temperature": 0.1}  # Low temp = more deterministic
        }, timeout=60)  # First call loads model into RAM (~30s), subsequent calls are fast

        if r.status_code == 200:
            response_text = r.json().get("response", "")
            return json.loads(response_text)
    except Exception as e:
        return {"error": str(e)}

    return {"error": "Ollama request failed"}


def rule_based_parse(text):
    """Fallback: simple regex parser when Ollama is not available."""
    text = text.lower().strip()

    # Priority / emergency commands (no AI needed)
    if text in ("stop", "halt", "freeze", "quick stop", "force stop",
                "emergency stop", "emergency", "quick sto", "stp", "s"):
        return {"commands": [{"action": "stop"}], "explanation": "EMERGENCY STOP", "method": "priority"}
    if text in ("come back", "return", "go home", "backtrack", "go back home",
                "return home", "back home", "home"):
        return {"commands": [{"action": "backtrack"}], "explanation": "Returning to start", "method": "priority"}
    if text in ("reset", "reset position", "restart"):
        return {"commands": [{"action": "reset"}], "explanation": "Position reset", "method": "priority"}

    # Simple commands (legacy, still works)
        return {"commands": [{"action": "stop"}], "explanation": "Emergency stop", "method": "rule"}
    if text in ("come back", "return", "go home", "backtrack", "go back home"):
        return {"commands": [{"action": "backtrack"}], "explanation": "Returning to start", "method": "rule"}
    if text in ("reset", "reset position"):
        return {"commands": [{"action": "reset"}], "explanation": "Position reset", "method": "rule"}

    # Parse direction + distance
    # Matches: "go forward 2 meters", "move left 50cm", "north 100", etc.
    # Longer names first so "northwest" matches before "north"
    directions = [
        ("northwest", -0.707, 0.707), ("northeast", 0.707, 0.707),
        ("southwest", -0.707, -0.707), ("southeast", 0.707, -0.707),
        ("forward", 0, 1), ("ahead", 0, 1), ("north", 0, 1), ("up", 0, 1),
        ("backward", 0, -1), ("south", 0, -1), ("down", 0, -1),
        ("left", -1, 0), ("west", -1, 0),
        ("right", 1, 0), ("east", 1, 0),
        ("back", 0, -1),
    ]

    commands = []
    # Split by "then", "and then", ","
    parts = re.split(r'\s+then\s+|\s+and\s+then\s+|,\s*then\s*|,\s*', text)

    for part in parts:
        part = part.strip()
        if not part:
            continue

        matched = False
        for dirname, dx, dy in directions:
            pattern = rf'(?:go\s+|move\s+)?{dirname}\s+(\d+\.?\d*)\s*(m|meter|meters|cm|centimeter|centimeters)?'
            m = re.search(pattern, part)
            if m:
                dist = float(m.group(1))
                unit = m.group(2) or "cm"
                if unit in ("m", "meter", "meters"):
                    dist *= 100

                x = round(dx * dist)
                y = round(dy * dist)
                commands.append({"action": "set_target", "x": x, "y": y})
                matched = True
                break

        # Check for backtrack phrases BEFORE bare direction match
        if not matched and re.search(r'come\s+back|return|go\s+home|backtrack|go\s+back\s+home', part):
            commands.append({"action": "backtrack"})
            matched = True

        if not matched:
            for dirname, dx, dy in directions:
                if dirname in part:
                    commands.append({"action": "set_target", "x": round(dx * 100), "y": round(dy * 100)})
                    matched = True
                    break

        if not matched and ("home" in part or "return" in part):
            commands.append({"action": "backtrack"})
            matched = True

    if commands:
        explanation = f"Parsed {len(commands)} command(s) from text"
        return {"commands": commands, "explanation": explanation, "method": "rule"}

    return {"error": "Could not understand. Try: 'go forward 2 meters' or 'come back'", "method": "rule"}


def translate(user_input):
    """
    Main entry: translate natural language to robot commands.
    Uses Ollama if available, falls back to rule-based.
    Returns dict with: commands, explanation, method, debug info.
    """
    start_time = time.time()
    result = {
        "input": user_input,
        "timestamp": time.strftime("%H:%M:%S"),
        "method": "unknown",
        "ollama_available": False,
        "commands": [],
        "explanation": "",
        "error": None,
        "duration_ms": 0,
    }

    # Check priority commands first (instant, no AI)
    priority = rule_based_parse(user_input)
    if priority.get("method") == "priority":
        result["commands"] = priority["commands"]
        result["explanation"] = priority["explanation"]
        result["method"] = "priority (instant)"
        result["duration_ms"] = round((time.time() - start_time) * 1000)
        return result

    # Try Ollama
    if check_ollama():
        result["ollama_available"] = True
        result["method"] = "ollama"

        ai_response = ask_ollama(user_input)

        if "error" not in ai_response:
            commands = ai_response.get("commands", [])
            # Validate: only allow known actions
            valid_actions = {"set_target", "backtrack", "stop", "reset"}
            validated = []
            for cmd in commands:
                action = cmd.get("action", "")
                if action in valid_actions:
                    validated.append(cmd)
                elif "x" in cmd or "y" in cmd or "distance" in cmd:
                    # LLM used wrong action name but has coordinates — fix it
                    x = cmd.get("x", 0)
                    y = cmd.get("y", 0)
                    validated.append({"action": "set_target", "x": x, "y": y})
                # else: skip unknown action

            if validated:
                # Post-validate: check if Ollama did diagonal math wrong
                # If user said diagonal but only one axis has a value, fix it
                input_lower = user_input.lower()
                diag_dirs = {
                    "northeast": (1, 1), "northwest": (-1, 1),
                    "southeast": (1, -1), "southwest": (-1, -1)
                }
                for dname, (sx, sy) in diag_dirs.items():
                    if dname in input_lower:
                        for cmd in validated:
                            if cmd.get("action") == "set_target":
                                x, y = cmd.get("x", 0), cmd.get("y", 0)
                                # If one axis is 0 but shouldn't be, fix diagonal
                                if (x == 0 and y != 0) or (y == 0 and x != 0):
                                    dist = abs(x) if x != 0 else abs(y)
                                    cmd["x"] = round(sx * dist * 0.707)
                                    cmd["y"] = round(sy * dist * 0.707)
                                    result["fixed_diagonal"] = True
                        break

                result["commands"] = validated
                result["explanation"] = ai_response.get("explanation", "")
                result["raw_response"] = ai_response
            else:
                # Ollama returned garbage, fall back to rules
                result["method"] = "rule (ollama output invalid)"
                fallback = rule_based_parse(user_input)
                result["commands"] = fallback.get("commands", [])
                result["explanation"] = fallback.get("explanation", "")
                result["error"] = fallback.get("error")
        else:
            # Ollama failed, fall back to rules
            result["method"] = "rule (ollama error)"
            result["ollama_error"] = ai_response["error"]
            fallback = rule_based_parse(user_input)
            result["commands"] = fallback.get("commands", [])
            result["explanation"] = fallback.get("explanation", "")
            result["error"] = fallback.get("error")
    else:
        # No Ollama, use rules
        result["method"] = "rule (ollama offline)"
        fallback = rule_based_parse(user_input)
        result["commands"] = fallback.get("commands", [])
        result["explanation"] = fallback.get("explanation", "")
        result["error"] = fallback.get("error")

    result["duration_ms"] = round((time.time() - start_time) * 1000)
    return result
