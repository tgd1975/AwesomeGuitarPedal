"""Unit test for generate_hid_display_table.py — idempotency check.

Runs the generator twice and asserts the second run reports
"already up to date." (no diff) — that's the contract TASK-338's
AC bar #1 demands.
"""

import pathlib
import subprocess
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parents[2]
GENERATOR = REPO / "scripts" / "generate_hid_display_table.py"
OUTPUT = REPO / "app" / "lib" / "constants" / "hid_display_table.dart"


class IdempotencyTest(unittest.TestCase):
    def test_generator_is_idempotent(self) -> None:
        # Force a regen so the first run always writes; the second
        # must then report "already up to date" with no file change.
        first = subprocess.run(
            [sys.executable, str(GENERATOR)],
            capture_output=True,
            text=True,
            check=True,
        )
        before = OUTPUT.read_bytes()

        second = subprocess.run(
            [sys.executable, str(GENERATOR)],
            capture_output=True,
            text=True,
            check=True,
        )
        after = OUTPUT.read_bytes()

        self.assertEqual(before, after, "second run rewrote the file")
        self.assertIn(
            "already up to date",
            second.stdout,
            f"expected idempotent run to short-circuit; stdout was: {second.stdout!r}",
        )

    def test_generator_uses_a_consistent_packed_key(self) -> None:
        text = OUTPUT.read_text(encoding="utf-8")
        # Sanity: Enter should be at packed (0x07 << 16) | 40 = 0x00070028.
        self.assertIn("0x00070028:", text)


if __name__ == "__main__":
    unittest.main()
