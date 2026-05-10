#!/usr/bin/env python3
"""
Lint platformio.ini.

Wraps `pio project config --lint` and turns its quiet warnings about
unknown keys into hard failures, because the bare command exits 0
even on typos like `boord =` instead of `board =`.

Failure modes caught:
  * Structural / syntax errors        — pio exits 1.
  * Bad value types (e.g. baud rate)  — pio exits 1.
  * Unknown keys (typos)              — pio prints "Warning ... Ignore
                                        unknown configuration option"
                                        and exits 0; we promote it to
                                        a failure.

Usage:
    python scripts/validate_platformio_ini.py [--project-dir DIR]

Exit codes:
    0  platformio.ini is clean.
    1  one or more issues; details printed to stderr.
    2  the `pio` executable could not be located.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent

CANDIDATE_PIO_PATHS = [
    Path.home() / ".platformio" / "penv" / "bin" / "pio",
    Path.home() / ".platformio" / "penv" / "Scripts" / "pio.exe",
]


def find_pio() -> str | None:
    on_path = shutil.which("pio")
    if on_path:
        # Sanity-check: the pio shim sometimes lingers without the
        # python module installed (e.g. after a pipx uninstall). If
        # `pio --version` fails, fall through to the candidate list.
        try:
            r = subprocess.run(
                [on_path, "--version"],
                capture_output=True, text=True, timeout=10,
            )
            if r.returncode == 0:
                return on_path
        except (subprocess.TimeoutExpired, OSError):
            pass

    for p in CANDIDATE_PIO_PATHS:
        if p.is_file() and os.access(p, os.X_OK):
            return str(p)
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--project-dir", "-d",
        default=str(REPO_ROOT),
        help="Project directory containing platformio.ini",
    )
    args = parser.parse_args()

    pio = find_pio()
    if not pio:
        print(
            "ERROR: `pio` executable not found. Install PlatformIO "
            "(`pip install platformio` or `pipx install platformio`).",
            file=sys.stderr,
        )
        return 2

    # Pass 1: plain `pio project config`. Catches structural errors
    # (missing section headers, malformed lines) and value-type errors
    # (e.g. non-integer upload_speed) with a non-zero exit. Note:
    # `--lint` SWALLOWS these errors and exits 0, so it cannot replace
    # this pass.
    structural_cmd = [pio, "project", "config", "-d", args.project_dir]
    try:
        result = subprocess.run(structural_cmd, capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        print("ERROR: `pio project config` timed out after 60s.", file=sys.stderr)
        return 1
    if result.returncode != 0:
        print("ERROR: platformio.ini failed structural validation:", file=sys.stderr)
        print(((result.stdout or "") + (result.stderr or "")).rstrip(), file=sys.stderr)
        return 1

    # Pass 2: `--lint` for unknown-key detection. Exits 0 even on
    # warnings, so we scan output for "Ignore unknown configuration
    # option" and promote it to an error.
    lint_cmd = [pio, "project", "config", "--lint", "-d", args.project_dir]
    try:
        result = subprocess.run(lint_cmd, capture_output=True, text=True, timeout=60)
    except subprocess.TimeoutExpired:
        print("ERROR: `pio project config --lint` timed out after 60s.", file=sys.stderr)
        return 1

    output = (result.stdout or "") + (result.stderr or "")
    bad_lines = [
        line for line in output.splitlines()
        if "Ignore unknown configuration option" in line
    ]
    if bad_lines:
        print("ERROR: platformio.ini contains unknown keys:", file=sys.stderr)
        for line in bad_lines:
            print(f"  {line}", file=sys.stderr)
        print(
            "Fix the typo, or — if the key is intentional — add it as a "
            "custom_* option (see PlatformIO docs).",
            file=sys.stderr,
        )
        return 1

    print(output.rstrip() or "platformio.ini OK.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
