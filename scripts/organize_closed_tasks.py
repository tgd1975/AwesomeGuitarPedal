#!/usr/bin/env python3
"""
Archive all closed tasks into a versioned release folder under tasks/archive/.

Usage:
  python scripts/organize_closed_tasks.py v0.3.0
  python scripts/organize_closed_tasks.py v0.3.0 --dry-run

The script:
1. Creates docs/developers/tasks/archive/<version>/
2. Moves every .md file from the closed/ directory into that archive folder.
3. Updates the OVERVIEW.md via update_task_overview.py.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys

CLOSED_DIR = "docs/developers/tasks/closed"
ARCHIVE_DIR = "docs/developers/tasks/archive"


def validate_version(version: str) -> bool:
    return bool(re.fullmatch(r"v\d+\.\d+\.\d+", version))


def main():
    parser = argparse.ArgumentParser(description="Archive closed tasks by release version")
    parser.add_argument("version", help="Release version tag, e.g. v0.3.0")
    parser.add_argument("--dry-run", action="store_true", help="Show what would happen without making changes")
    args = parser.parse_args()

    if not validate_version(args.version):
        print(f"Error: version must be in the form vX.Y.Z, got: {args.version}", file=sys.stderr)
        sys.exit(1)

    target_dir = os.path.join(ARCHIVE_DIR, args.version)

    if not os.path.isdir(CLOSED_DIR):
        print(f"{CLOSED_DIR} does not exist — nothing to archive.")
        return

    md_files = [
        f for f in sorted(os.listdir(CLOSED_DIR))
        if f.endswith(".md") and os.path.isfile(os.path.join(CLOSED_DIR, f))
    ]

    if not md_files:
        print("No .md files found in the closed/ directory — nothing to archive.")
        return

    print(f"Target directory: {target_dir}")
    print(f"Files to archive ({len(md_files)}):")
    for f in md_files:
        print(f"  {f}")

    if args.dry_run:
        print("Dry run — no changes made.")
        return

    os.makedirs(target_dir, exist_ok=True)

    for fname in md_files:
        src = os.path.join(CLOSED_DIR, fname)
        dst = os.path.join(target_dir, fname)
        shutil.move(src, dst)
        print(f"Archived: {fname}")

    script_dir = os.path.dirname(os.path.abspath(__file__))
    overview_script = os.path.join(script_dir, "update_task_overview.py")
    subprocess.run([sys.executable, overview_script], check=True)

    print(f"Done — {len(md_files)} tasks archived under {target_dir}")


if __name__ == "__main__":
    main()
