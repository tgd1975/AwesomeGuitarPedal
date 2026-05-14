"""Tests for scripts/generate_wiring_config_header.py.

Covers the pieces that would silently produce a broken wiring-test
firmware: missing input, malformed JSON, raw-string-delimiter
collisions, and the basic happy path of round-tripping the JSON
through the generated header.
"""

import json
import pathlib
import subprocess
import sys
import tempfile
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]
GENERATOR = REPO / "scripts" / "generate_wiring_config_header.py"


def run(args, env=None):
    return subprocess.run(
        [sys.executable, str(GENERATOR), *args],
        capture_output=True,
        text=True,
        env=env,
    )


class GenerateWiringConfigHeaderTest(unittest.TestCase):
    def test_happy_path_embeds_json_and_filename(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            cfg = tmp / "config.json"
            payload = {"hardware": "esp32", "ledPower": 2}
            cfg.write_text(json.dumps(payload, indent=2))
            out = tmp / "wiring_config_embedded.h"

            result = run(["--config", str(cfg), "--output", str(out)])
            self.assertEqual(result.returncode, 0, result.stderr)

            text = out.read_text()
            self.assertIn('CONFIG_FILENAME = "config.json"', text)
            self.assertIn("R\"WIRING_JSON(", text)
            self.assertIn('"hardware": "esp32"', text)
            self.assertIn('"ledPower": 2', text)

    def test_missing_input_fails_cleanly(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp) / "out.h"
            result = run(
                ["--config", "/nonexistent/config.json", "--output", str(out)]
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("config not readable", result.stderr)
            self.assertFalse(out.exists())

    def test_malformed_json_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            cfg = tmp / "bad.json"
            cfg.write_text("{ not valid json")
            out = tmp / "out.h"

            result = run(["--config", str(cfg), "--output", str(out)])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("not valid JSON", result.stderr)
            self.assertFalse(out.exists())

    def test_delimiter_collision_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            cfg = tmp / "evil.json"
            cfg.write_text('{"WIRING_JSON": 1}')
            out = tmp / "out.h"

            result = run(["--config", str(cfg), "--output", str(out)])
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("WIRING_JSON", result.stderr)
            self.assertFalse(out.exists())

    def test_env_var_fallback(self) -> None:
        import os

        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            cfg = tmp / "config.json"
            cfg.write_text('{"hardware":"esp32"}')
            out = tmp / "out.h"

            env = os.environ.copy()
            env["ASP_WIRING_CONFIG"] = str(cfg)
            result = run(["--output", str(out)], env=env)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('"hardware":"esp32"', out.read_text())

    def test_missing_env_var_when_no_config_arg(self) -> None:
        import os

        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp) / "out.h"
            env = os.environ.copy()
            env.pop("ASP_WIRING_CONFIG", None)
            result = run(["--output", str(out)], env=env)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("ASP_WIRING_CONFIG", result.stderr)


if __name__ == "__main__":
    unittest.main()
