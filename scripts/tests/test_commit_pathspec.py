"""Tests for scripts/commit-pathspec.sh — the wrapper behind /commit.

Covers the post-TASK-329 fail-fast-on-untracked behaviour. Each test
runs the wrapper inside a throwaway git repo so it can exercise
real-git semantics without touching the host repo. The pre-commit
hook is bypassed via ``ASP_COMMIT_BYPASS`` so the tests focus on the
wrapper's own logic, not the hook's.
"""

from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory


REPO_ROOT = Path(__file__).resolve().parents[2]
WRAPPER = REPO_ROOT / "scripts" / "commit-pathspec.sh"


class TestCommitPathspecWrapper(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = TemporaryDirectory()
        self.work = Path(self.tmp.name)
        self._git("init", "-q", "-b", "main")
        self._git("config", "user.email", "test@example.com")
        self._git("config", "user.name", "Test")
        # Seed a tracked file so the repo has a HEAD.
        (self.work / "seed.txt").write_text("seed\n")
        self._git("add", "seed.txt")
        self._git("commit", "-q", "-m", "seed")

    def tearDown(self) -> None:
        self.tmp.cleanup()

    def _git(self, *args: str) -> subprocess.CompletedProcess:
        return subprocess.run(
            ["git", *args],
            cwd=self.work,
            check=True,
            capture_output=True,
            text=True,
        )

    def _run_wrapper(self, *args: str) -> subprocess.CompletedProcess:
        env = os.environ.copy()
        # Bypass the provenance hook — we only test the wrapper here.
        env["ASP_COMMIT_BYPASS"] = "test"
        return subprocess.run(
            ["bash", str(WRAPPER), *args],
            cwd=self.work,
            env=env,
            capture_output=True,
            text=True,
        )

    # ---- rejection paths -------------------------------------------------

    def test_rejects_untracked_pathspec_entry(self) -> None:
        # New file on disk but never `git add`ed.
        (self.work / "new.txt").write_text("hello\n")
        result = self._run_wrapper("msg", "new.txt")
        self.assertEqual(
            result.returncode,
            2,
            f"expected exit 2, got {result.returncode}: stderr={result.stderr}",
        )
        self.assertIn("untracked pathspec", result.stderr)
        self.assertIn("new.txt", result.stderr)

    def test_wrapper_does_not_git_add_untracked_entry(self) -> None:
        # Confirm that on the rejection path, the wrapper did NOT silently
        # `git add` the file. The index must remain untouched.
        (self.work / "new.txt").write_text("hello\n")
        # Sanity: file is untracked before.
        status_before = self._git("status", "--porcelain").stdout
        self.assertIn("?? new.txt", status_before)
        result = self._run_wrapper("msg", "new.txt")
        self.assertNotEqual(result.returncode, 0)
        # File still untracked after — wrapper did not add it.
        status_after = self._git("status", "--porcelain").stdout
        self.assertIn("?? new.txt", status_after)

    def test_rejects_empty_pathspec(self) -> None:
        result = self._run_wrapper("just a message")
        self.assertNotEqual(result.returncode, 0)
        # The "<2 args" branch fires before the empty-pathspec branch when
        # only one arg is passed; either error message is acceptable.
        self.assertTrue(
            "Usage" in result.stderr or "no files in pathspec" in result.stderr,
            f"unexpected stderr: {result.stderr}",
        )

    # ---- success paths ---------------------------------------------------

    def test_accepts_tracked_modified_pathspec(self) -> None:
        # Modify the seeded file but do NOT `git add` it — pathspec form
        # picks up the working-tree contents directly.
        (self.work / "seed.txt").write_text("seed\nmodified\n")
        result = self._run_wrapper("modify seed", "seed.txt")
        self.assertEqual(
            result.returncode,
            0,
            f"expected success, got: stderr={result.stderr} stdout={result.stdout}",
        )
        # The new commit should contain seed.txt with the modification.
        log = self._git("log", "--name-only", "-1", "--format=").stdout.strip()
        self.assertEqual(log, "seed.txt")

    def test_accepts_tracked_after_explicit_git_add(self) -> None:
        # The /commit skill is now responsible for `git add`ing untracked
        # entries before invoking the wrapper. Simulate that and confirm
        # the wrapper accepts the (now-tracked) file.
        (self.work / "added.txt").write_text("body\n")
        self._git("add", "added.txt")
        result = self._run_wrapper("add a file", "added.txt")
        self.assertEqual(
            result.returncode,
            0,
            f"expected success, got: stderr={result.stderr} stdout={result.stdout}",
        )

    def test_accepts_rename_with_both_paths_in_pathspec(self) -> None:
        # TASK-347: after `git mv A B`, both A and B must be accepted as
        # pathspec entries so the resulting commit records a rename
        # rather than an orphan add (B) with a dangling source-side
        # deletion (A).
        (self.work / "old.txt").write_text("renamed body\n")
        self._git("add", "old.txt")
        self._git("commit", "-q", "-m", "seed old.txt")
        self._git("mv", "old.txt", "new.txt")
        result = self._run_wrapper("rename old to new", "old.txt", "new.txt")
        self.assertEqual(
            result.returncode,
            0,
            f"expected success, got: stderr={result.stderr} stdout={result.stdout}",
        )
        # The commit should record both sides — old.txt deleted, new.txt
        # added. With rename detection, git log surfaces it as a rename;
        # name-status with -M confirms it.
        log = self._git(
            "log", "-1", "--name-status", "-M", "--format="
        ).stdout.strip()
        self.assertTrue(
            log.startswith("R"),
            f"expected rename status (R…) in log, got: {log!r}",
        )
        self.assertIn("old.txt", log)
        self.assertIn("new.txt", log)
        # The working tree must be clean — no orphan ` D old.txt`.
        status = self._git("status", "--porcelain").stdout
        self.assertEqual(
            status,
            "",
            f"expected clean working tree after rename commit, got: {status!r}",
        )

    def test_accepts_on_disk_deletion_of_tracked_file(self) -> None:
        # TASK-347: deleting a tracked file from disk (without `git rm`)
        # must be commitable via the wrapper. The path is in HEAD and
        # in the index (as the unmodified entry), so the wrapper accepts
        # it; pathspec form picks up the working-tree absence and the
        # commit records the deletion.
        (self.work / "doomed.txt").write_text("goodbye\n")
        self._git("add", "doomed.txt")
        self._git("commit", "-q", "-m", "seed doomed.txt")
        (self.work / "doomed.txt").unlink()
        result = self._run_wrapper("delete doomed", "doomed.txt")
        self.assertEqual(
            result.returncode,
            0,
            f"expected success, got: stderr={result.stderr} stdout={result.stdout}",
        )
        log = self._git(
            "log", "-1", "--name-status", "--format="
        ).stdout.strip()
        self.assertTrue(
            log.startswith("D"),
            f"expected deletion status (D…) in log, got: {log!r}",
        )
        self.assertIn("doomed.txt", log)
        status = self._git("status", "--porcelain").stdout
        self.assertEqual(
            status,
            "",
            f"expected clean working tree after deletion commit, got: {status!r}",
        )


if __name__ == "__main__":
    unittest.main()
