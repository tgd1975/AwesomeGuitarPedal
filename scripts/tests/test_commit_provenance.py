"""Tests for the commit-provenance section of scripts/pre-commit (EPIC-022).

Exercises only the provenance check at the top of the hook. To do so, the
test extracts that section into a standalone snippet (everything between
the start marker and `echo -e "${GREEN}/commit provenance OK.${NC}"`) and
runs it as a fresh shell against a throwaway git directory. This avoids
running the full pre-commit chain (clang-tidy, tests, mermaid) and keeps
the test fast.
"""

from __future__ import annotations

import os
import re
import subprocess
import textwrap
import time
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory


REPO_ROOT = Path(__file__).resolve().parents[2]
PRECOMMIT = REPO_ROOT / "scripts" / "pre-commit"


def extract_provenance_snippet() -> str:
    """Extract just the provenance-check block from scripts/pre-commit.

    The block starts after the color setup and ends with the
    "/commit provenance OK." log line. We replace `git rev-parse --git-dir`
    with $GIT_DIR (set in the test fixture) so we don't depend on running
    inside a real git repo.
    """
    text = PRECOMMIT.read_text()
    match = re.search(
        r"# Commit-provenance check.*?\necho -e \"\$\{GREEN\}/commit provenance OK\.\$\{NC\}\"",
        text,
        flags=re.DOTALL,
    )
    if not match:
        raise RuntimeError("could not find provenance block in scripts/pre-commit")
    snippet = match.group(0)
    # Replace git-rev-parse with the env var so the snippet runs without git.
    snippet = snippet.replace(
        'GIT_DIR="$(git rev-parse --git-dir)"',
        'GIT_DIR="${GIT_DIR}"',
    )
    # And rev-parse for branch — make it best-effort.
    snippet = snippet.replace(
        '"$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo unknown)"',
        '"test-branch"',
    )
    return "set -euo pipefail\nRED=''\nGREEN=''\nYELLOW=''\nNC=''\n" + snippet


class TestCommitProvenance(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = TemporaryDirectory()
        self.git_dir = Path(self.tmp.name) / ".git"
        self.git_dir.mkdir()
        self.token_path = self.git_dir / "asp-commit-token"
        self.bypass_log = self.git_dir / "asp-commit-bypass.log"
        self.snippet = extract_provenance_snippet()

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def run_hook(
        self, *, env_extra: dict[str, str] | None = None
    ) -> subprocess.CompletedProcess:
        env = os.environ.copy()
        env["GIT_DIR"] = str(self.git_dir)
        env.pop("ASP_COMMIT_BYPASS", None)
        if env_extra:
            env.update(env_extra)
        return subprocess.run(
            ["bash", "-c", self.snippet],
            env=env,
            capture_output=True,
            text=True,
        )

    def write_token(
        self, *, pid: int | None = None, age_seconds: int = 0, body: str | None = None
    ) -> None:
        if body is not None:
            self.token_path.write_text(body)
            return
        actual_pid = pid if pid is not None else os.getpid()
        nonce = "deadbeef" * 4
        ts = int(time.time()) - age_seconds
        self.token_path.write_text(f"{actual_pid} {nonce} {ts}\n")

    # ---- rejection paths -------------------------------------------------

    def test_rejects_when_no_token_and_no_bypass(self) -> None:
        result = self.run_hook()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("did not flow through /commit", result.stderr)

    def test_rejects_malformed_token(self) -> None:
        self.write_token(body="this is not a valid token line\n")
        result = self.run_hook()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("malformed", result.stderr)
        self.assertFalse(self.token_path.exists(), "malformed token must be cleaned up")

    def test_rejects_stale_token(self) -> None:
        # Token from 2 minutes ago with the current PID — would otherwise
        # pass PID-ancestry, but TTL is 60s.
        self.write_token(age_seconds=120)
        result = self.run_hook()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("stale", result.stderr)
        self.assertFalse(self.token_path.exists(), "stale token must be cleaned up")

    def test_rejects_unrelated_pid(self) -> None:
        # PID 1 (init) is virtually never an ancestor of our test process.
        self.write_token(pid=1)
        result = self.run_hook()
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("not an ancestor", result.stderr)
        self.assertFalse(self.token_path.exists())

    # ---- success paths ---------------------------------------------------

    def test_accepts_valid_token_from_current_process(self) -> None:
        # Token written with our own PID — bash subshell PID is a descendant
        # of os.getpid(), so the ancestry walk should find us.
        self.write_token()
        result = self.run_hook()
        self.assertEqual(
            result.returncode,
            0,
            f"expected success, got: stderr={result.stderr} stdout={result.stdout}",
        )
        self.assertFalse(
            self.token_path.exists(),
            "valid token must be consumed (deleted) on success",
        )

    def test_bypass_with_reason_succeeds_and_logs(self) -> None:
        result = self.run_hook(env_extra={"ASP_COMMIT_BYPASS": "rebase conflict"})
        self.assertEqual(result.returncode, 0)
        self.assertTrue(
            self.bypass_log.exists(), "bypass must append to .git/asp-commit-bypass.log"
        )
        log_content = self.bypass_log.read_text()
        self.assertIn("rebase conflict", log_content)
        self.assertIn("pending", log_content, "SHA placeholder for post-commit backfill")

    def test_empty_bypass_does_not_authorise(self) -> None:
        # Empty string in env — the check should treat as unset.
        result = self.run_hook(env_extra={"ASP_COMMIT_BYPASS": ""})
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("did not flow through /commit", result.stderr)
        self.assertFalse(self.bypass_log.exists(), "empty bypass must not log")

    def test_bypass_drops_stale_token(self) -> None:
        # If a token exists but bypass is set, the bypass path should drop
        # the token so a later commit can't piggy-back on it.
        self.write_token(age_seconds=10)
        result = self.run_hook(env_extra={"ASP_COMMIT_BYPASS": "manual surgery"})
        self.assertEqual(result.returncode, 0)
        self.assertFalse(self.token_path.exists(), "bypass must drop the token file")


if __name__ == "__main__":
    unittest.main()
