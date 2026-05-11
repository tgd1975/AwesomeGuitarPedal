"""Unit tests for organize_closed_tasks.py."""

from __future__ import annotations

import io
import os
import pathlib
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from unittest import mock

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent.parent))
import organize_closed_tasks as oct  # noqa: E402


class OrganizeClosedTasksTests(unittest.TestCase):
    def _run_with_cwd(self, cwd: pathlib.Path, version: str = "v9.9.9") -> str:
        buf = io.StringIO()
        argv = ["organize_closed_tasks.py", version]
        old_cwd = os.getcwd()
        os.chdir(cwd)
        try:
            with mock.patch.object(sys, "argv", argv), redirect_stdout(buf):
                oct.main()
        finally:
            os.chdir(old_cwd)
        return buf.getvalue()

    def test_missing_closed_dir_is_noop(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            out = self._run_with_cwd(pathlib.Path(tmp))
            self.assertIn("does not exist", out)
            self.assertFalse((pathlib.Path(tmp) / oct.ARCHIVE_DIR).exists())

    def test_empty_closed_dir_is_noop(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            (pathlib.Path(tmp) / oct.CLOSED_DIR).mkdir(parents=True)
            out = self._run_with_cwd(pathlib.Path(tmp))
            self.assertIn("nothing to archive", out)
            self.assertFalse((pathlib.Path(tmp) / oct.ARCHIVE_DIR).exists())


if __name__ == "__main__":
    unittest.main()
