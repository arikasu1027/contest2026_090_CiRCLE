#!/usr/bin/env python3
import argparse, json, os, re, sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional


@dataclass
class ConfigEntry:
    name: str = ""
    value: Optional[str] = None
    enabled: bool = False
    type: str = "unknown"
    defined_in: str = ""
    depends_on: list = field(default_factory=list)
    selects: list = field(default_factory=list)
    implies: list = field(default_factory=list)
    help_text: str = ""
    default: str = ""


@dataclass
class QueryResult:
    status: str = ""
    config_name: str = ""
    value: Optional[str] = None
    enabled: bool = False
    dependencies: list = field(default_factory=list)
    selects: list = field(default_factory=list)
    implies: list = field(default_factory=list)
    defined_in: str = ""
    help_text: str = ""
    fuzzy_matches: Optional[list] = None
    suggestion: str = ""


def find_dotconfig(workspace, build_dir):
    for p in [
        Path(build_dir) / ".config",
        Path(workspace) / "build" / ".config",
        Path(workspace) / ".config",
        Path(workspace) / "out" / ".config",
    ]:
        if p.is_file():
            return p
    return None


def parse_dotconfig(path):
    cfg = {}
    with open(path, encoding="utf-8", errors="ignore") as f:
        for line in f:
            line = line.strip()
            m = re.match(r"^# (CONFIG_\w+) is not set$", line)
            if m:
                cfg[m.group(1)] = "n"
                continue
            m = re.match(r"^(CONFIG_\w+)=(.*)$", line)
            if m:
                cfg[m.group(1)] = m.group(2)
    return cfg


_KCACHE = {}


def find_kconfig_files(workspace):
    if workspace in _KCACHE:
        return _KCACHE[workspace]
    files = []
    skip = {".git", "build", "out", "__pycache__", ".repo", "prebuilts"}
    for root, dirs, names in os.walk(Path(workspace)):
        dirs[:] = [d for d in dirs if d not in skip]
        for n in names:
            if n == "Kconfig" or n.startswith("Kconfig.") or n.endswith(".kconfig"):
                files.append(Path(root) / n)
    _KCACHE[workspace] = files
    return files


def parse_kconfig_entry(filepath, target):
    try:
        content = filepath.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        return None
    pat = re.compile(
        rf"^(?:menu)?config\s+{re.escape(target)}\s*$",
        re.MULTILINE,
    )
    m = pat.search(content)
    if not m:
        return None
    rest = content[m.start():]
    blk_re = re.compile(
        r"^(?:menu)?config\s|^(?:end)?menu\b|^(?:end)?choice\b|^source\s",
        re.MULTILINE,
    )
    m2 = blk_re.search(rest, pos=m.end() - m.start())
    block = rest[:m2.start()] if m2 else rest
    entry = ConfigEntry(name="CONFIG_" + target, defined_in=str(filepath))

    tm = re.search(r"^\s*(bool|tristate|int|hex|string)\s+(.*)$", block, re.MULTILINE)
    if tm:
        entry.type = tm.group(1)
    for dm in re.finditer(r"^\s*depends on\s+(.+)$", block, re.MULTILINE):
        for d in re.findall(r"(?:CONFIG_)?(\w+)", dm.group(1)):
            dep = d if d.startswith("CONFIG_") else "CONFIG_" + d
            if dep not in entry.depends_on:
                entry.depends_on.append(dep)
    for sm in re.finditer(r"^\s*select\s+(\w+)", block, re.MULTILINE):
        s = sm.group(1)
        s = s if s.startswith("CONFIG_") else "CONFIG_" + s
        if s not in entry.selects:
            entry.selects.append(s)
    for im in re.finditer(r"^\s*imply\s+(\w+)", block, re.MULTILINE):
        s = im.group(1)
        s = s if s.startswith("CONFIG_") else "CONFIG_" + s
        if s not in entry.implies:
            entry.implies.append(s)
    defm = re.search(r"^\s*default\s+(.+)$", block, re.MULTILINE)
    if defm:
        entry.default = defm.group(1).strip()
    hm = re.search(r"^\s*help\s*$", block, re.MULTILINE)
    if hm:
        lines = []
        for ln in block[hm.end():].splitlines():
            if ln and not ln[0].isspace():
                break
            s = ln.strip()
            if s:
                lines.append(s)
        entry.help_text = "\n".join(lines[:5])
    return entry


def fuzzy_search(workspace, query, max_results=20):
    ql = query.lower()
    results = []
    seen = set()
    cpat = re.compile(r"^(?:menu)?config\s+(\w+)", re.MULTILINE)
    for kf in find_kconfig_files(workspace):
        try:
            content = kf.read_text(encoding="utf-8", errors="ignore")
        except OSError:
            continue
        for m in cpat.finditer(content):
            name = m.group(1)
            cfull = "CONFIG_" + name
            if cfull in seen:
                continue
            nm = ql in name.lower()
            pm = False
            if not nm:
                bs = m.end()
                be = content.find("\nconfig", bs)
                if be == -1:
                    be = min(bs + 2000, len(content))
                if ql in content[bs:be].lower():
                    pm = True
            if nm or pm:
                seen.add(cfull)
                bs = m.end()
                be = content.find("\nconfig", bs)
                if be == -1:
                    be = min(bs + 2000, len(content))
                blk = content[bs:be]
                pr = re.search(
                    r'^\s*(?:bool|tristate|int|hex|string)\s+"?(.+?)"?\s*$',
                    blk, re.MULTILINE,
                )
                prompt = pr.group(1).strip('"') if pr else ""
                results.append({
                    "config": cfull,
                    "prompt": prompt[:80],
                    "defined_in": str(kf),
                    "match_type": "name" if nm else "description",
                })
                if len(results) >= max_results:
                    return results
    results.sort(key=lambda x: (0 if x["match_type"] == "name" else 1, x["config"]))
    return results[:max_results]


def gen_suggestion(config_name, entry):
    lines = []
    if entry and entry.depends_on:
        lines.append("# depends on (must be enabled):")
        for dep in entry.depends_on:
            lines.append("# " + dep + "=y")
    if entry and entry.type in ("int", "hex", "string"):
        dv = entry.default or ("0" if entry.type != "string" else "<value>")
        lines.append(config_name + "=" + dv)
    else:
        lines.append(config_name + "=y")
    if entry and entry.selects:
        lines.append("# auto-selected:")
        for sel in entry.selects:
            lines.append("# " + sel + "=y")
    return "\n".join(lines)


def query_config(workspace, query, build_dir=None, fuzzy=False, max_results=20):
    if not build_dir:
        build_dir = os.path.join(workspace, "build")
    query = query.strip()
    cname = query if query.startswith("CONFIG_") else "CONFIG_" + query
    bare = cname.replace("CONFIG_", "")
    dcp = find_dotconfig(workspace, build_dir)
    dotcfg = parse_dotconfig(dcp) if dcp else {}

    if cname in dotcfg:
        val = dotcfg[cname]
        enabled = val not in ("n", "0", "")
        entry = None
        for kf in find_kconfig_files(workspace):
            entry = parse_kconfig_entry(kf, bare)
            if entry:
                break
        r = QueryResult(
            status="found", config_name=cname, value=val, enabled=enabled,
            defined_in=entry.defined_in if entry else "",
            help_text=entry.help_text if entry else "",
        )
        if entry:
            r.dependencies = entry.depends_on
            r.selects = entry.selects
            r.implies = entry.implies
        if not enabled:
            r.suggestion = gen_suggestion(cname, entry)
        return r

    entry = None
    for kf in find_kconfig_files(workspace):
        entry = parse_kconfig_entry(kf, bare)
        if entry:
            break
    if entry:
        return QueryResult(
            status="not_found", config_name=cname,
            defined_in=entry.defined_in, help_text=entry.help_text,
            dependencies=entry.depends_on, selects=entry.selects,
            implies=entry.implies,
            suggestion=gen_suggestion(cname, entry),
        )

    matches = fuzzy_search(workspace, bare, max_results)
    r = QueryResult(status="fuzzy_match", config_name=cname)
    if matches:
        r.fuzzy_matches = matches
        r.suggestion = "# found %d related configs" % len(matches)
    else:
        r.suggestion = "# no config found for: %s" % query
    return r


def fmt(result):
    o = {"status": result.status, "config": result.config_name}
    if result.value is not None:
        o["value"] = result.value
    if result.enabled:
        o["enabled"] = True
    if result.defined_in:
        o["defined_in"] = result.defined_in
    if result.dependencies:
        o["depends_on"] = result.dependencies
    if result.selects:
        o["selects"] = result.selects
    if result.implies:
        o["implies"] = result.implies
    if result.help_text:
        o["help"] = result.help_text[:200]
    if result.fuzzy_matches:
        o["matches"] = result.fuzzy_matches
    if result.suggestion:
        o["defconfig_snippet"] = result.suggestion
    return json.dumps(o, ensure_ascii=False, indent=2)


def main():
    ap = argparse.ArgumentParser(description="OpenVela Kconfig Query")
    ap.add_argument("-q", "--query", required=True)
    ap.add_argument("-w", "--workspace", default=".")
    ap.add_argument("-b", "--build-dir", default=None)
    ap.add_argument("-f", "--fuzzy", action="store_true")
    ap.add_argument("-n", "--max-results", type=int, default=20)
    a = ap.parse_args()
    print(fmt(query_config(
        workspace=a.workspace, query=a.query,
        build_dir=a.build_dir, fuzzy=a.fuzzy,
        max_results=a.max_results,
    )))


if __name__ == "__main__":
    main()
