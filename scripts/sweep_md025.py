#!/usr/bin/env python3
"""TASK-369: Strip redundant body-level H1 from files where frontmatter title is the H1."""
from __future__ import annotations

import sys
from pathlib import Path


def strip_body_h1(path):
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)

    if not lines or not lines[0].startswith("---"):
        return False

    fm_end = None
    for i in range(1, len(lines)):
        if lines[i].rstrip("\n") == "---":
            fm_end = i
            break
    if fm_end is None:
        return False

    h1_idx = None
    for j in range(fm_end + 1, len(lines)):
        s = lines[j].lstrip()
        if s.startswith("# ") and not s.startswith("## "):
            h1_idx = j
            break

    if h1_idx is None:
        return False

    drop = 1
    if h1_idx + 1 < len(lines) and lines[h1_idx + 1].strip() == "":
        drop = 2

    new = "".join(lines[:h1_idx] + lines[h1_idx + drop:])
    if new != text:
        path.write_text(new, encoding="utf-8")
        return True
    return False


def main(argv):
    if not argv:
        print("usage: sweep_md025.py <file> [<file> ...]", file=sys.stderr)
        return 2
    changed = 0
    for arg in argv:
        p = Path(arg)
        if not p.exists():
            print("  SKIP (missing): " + arg, file=sys.stderr)
            continue
        if strip_body_h1(p):
            print("  STRIPPED H1: " + arg)
            changed += 1
        else:
            print("  no-op:       " + arg)
    print("Done. " + str(changed) + " modified.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
