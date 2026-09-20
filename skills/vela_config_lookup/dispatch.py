#!/usr/bin/env python3
import json, re, subprocess, sys
from pathlib import Path

SKILL_DIR = Path(__file__).parent
SCRIPT = SKILL_DIR / "vela_config_query.py"


def extract_config_names(user_input):
    explicit = re.findall(r"CONFIG_[A-Z0-9_]+", user_input)
    if explicit:
        return list(set(explicit))
    caps = re.findall(r"\b([A-Z][A-Z0-9_]{2,})\b", user_input)
    if caps:
        return list(set(caps))
    quoted = re.findall(r"[\x22\x27](.*?)[\x22\x27]", user_input)
    if quoted:
        return [q.strip() for q in quoted if q.strip()]
    return []


def call_sub_agent(user_input, workspace=".", build_dir=None):
    names = extract_config_names(user_input)
    if not names:
        return json.dumps(
            {"error": "no config name found", "hint": "provide CONFIG_XXX or keyword"},
            ensure_ascii=False,
        )
    results = []
    for name in names:
        cmd = [sys.executable, str(SCRIPT), "-q", name, "-w", workspace]
        if build_dir:
            cmd.extend(["-b", build_dir])
        try:
            p = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
            if p.returncode == 0:
                results.append(json.loads(p.stdout.strip()))
            else:
                results.append({"config": name, "error": p.stderr.strip()[:200]})
        except subprocess.TimeoutExpired:
            results.append({"config": name, "error": "timeout"})
        except json.JSONDecodeError:
            results.append({"config": name, "error": "parse error"})
    if len(results) == 1:
        return json.dumps(results[0], ensure_ascii=False, indent=2)
    return json.dumps(results, ensure_ascii=False, indent=2)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("usage: python3 dispatch.py 'query' /path/to/openvela")
        sys.exit(1)
    ws = sys.argv[2] if len(sys.argv) > 2 else "."
    print(call_sub_agent(sys.argv[1], workspace=ws))
