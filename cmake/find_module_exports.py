import re
import sys
from pathlib import Path

MODULE_REGEX = re.compile(
    r'^\s*export\s+module\s+([a-zA-Z0-9_.:]+)\s*;',
    re.MULTILINE
)

def extract_module_name(path: Path):
    text = path.read_text(encoding="utf-8")
    m = MODULE_REGEX.findall(text)
    if not m:
        print(f"[WARN] No module found in {path}")
        return None
    if len(m) > 1:
        print(f"[WARN] Multiple modules in {path}, using first")

    return m[0]

def main():
    if len(sys.argv) < 4:
        print("Missing args (bug?)")
        sys.exit(1)

    template = Path(sys.argv[1])
    out = Path(sys.argv[2])
    files = [Path(p) for p in sys.argv[3:]]

    modules = []
    for f in files:
        name = extract_module_name(f)
        if name:
            modules.append(name)
    modules = sorted(set(modules))
    print(f"Found modules: {modules}")

    lines = []
    for m in modules:
        lines.append(f"import {m};")
    lines.append("")
    lines = "\n".join(lines)

    template_text = template.read_text(encoding="utf-8")
    final_text = lines + "\n" + template_text
    out.write_text(final_text, encoding="utf-8")

if __name__ == "__main__":
    main()
