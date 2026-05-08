---
name: release-branch
description: Squash a feature/refactoring branch, merge it into main, update CHANGELOG, and cut a release
---

# release-branch

Squash-merge the current branch into `main` and cut a release in one guided flow.

Invoke as `/release-branch vX.Y.Z`. If no version is given, read the current version from
`platformio.ini` and ask what the new version should be.

## Steps

### Phase 1 — Prepare the branch

1. **Verify state**: run `git status`. Stop if uncommitted changes exist. Run
   `git branch --show-current`. Stop if already on `main`.

2. **Show branch summary**: print the branch name and every commit ahead of `main`:

   ```bash
   git log main..HEAD --oneline
   ```

3. **Confirm version**: read current version from `platformio.ini`. Show it and ask the
   user to confirm the target release version if not supplied as an argument.

### Phase 2 — Update CHANGELOG

1. **Populate `[Unreleased]`**: read `CHANGELOG.md`. Find the `## [Unreleased]` section.

   - If the section is **empty**, generate candidate entries from the commit log:

     ```bash
     git log main..HEAD --pretty=format:"- %s"
     ```

     Group entries under appropriate sub-headings (`### Added`, `### Changed`,
     `### Fixed`) based on conventional-commit prefixes (`feat`/`fix`/`refactor`/`chore`).
     Show the draft to the user and ask for confirmation or edits before writing.

     **Important — what belongs under `### Fixed`:** only list a defect under
     `### Fixed` if it shipped in a prior release or comes from the environment
     (OS, dependency, hardware). If a bug was introduced *and* resolved within the
     same release cycle, it never reached users — fold it silently into the
     corresponding `### Added` entry (the feature simply works) and leave it out
     of `### Fixed`. Same rule for `### Changed`: don't list a behaviour that was
     introduced and revised within this cycle. To audit candidates, cross-check
     each `fix:` / defect commit against the previous release's CHANGELOG — if the
     affected feature is new in this cycle, the fix isn't a fix.

   - If the section already has content, show it and ask the user to confirm it is
     complete before proceeding.

2. **Write CHANGELOG**: replace the `## [Unreleased]` heading with:

   ```
   ## [Unreleased]

   ---

   ## [vX.Y.Z] — YYYY-MM-DD
   ```

   Keep the existing `[Unreleased]` content under the new versioned heading.
   Use today's date (`currentDate` from context, or `date +%Y-%m-%d`).

### Phase 3 — Squash and push

1. **Squash commits**: propose a squash commit message based on the branch name and
   version (e.g. `"release: squash feature/improvements for vX.Y.Z"`). Confirm with the
   user, then:

   ```bash
   BASE=$(git merge-base HEAD main)
   git reset --soft $BASE
   git add CHANGELOG.md
   ASP_COMMIT_BYPASS="release-branch: squash commit" \
       git commit -m "<confirmed message>"
   ```

   The bypass is required because the squash commit's file set is
   "everything in the branch" — not enumerable as a flat pathspec
   list. See [docs/developers/COMMIT_POLICY.md](../../../docs/developers/COMMIT_POLICY.md);
   the bypass is logged to `.git/asp-commit-bypass.log`.

2. **Push branch**:

   ```bash
   git push --force-with-lease origin <branch>
   ```

3. **Open PR**: before running `gh pr create`, verify `gh` is authenticated:

   ```bash
   gh auth status
   ```

   If the output contains "not logged in" or exits non-zero, stop and ask the user to run:

   ```
   gh auth login
   ```

   Wait for confirmation that login succeeded, then proceed. Create a pull request targeting `main`:

   ```bash
   gh pr create --title "Release vX.Y.Z" --body "$(cat <<'EOF'
   ## Summary
   <CHANGELOG [vX.Y.Z] content here>

   🤖 Generated with [Claude Code](https://claude.com/claude-code)
   EOF
   )"
   ```

   Print the PR URL.

4. **Merge the PR via `gh`** (preferred over the GitHub UI — keeps the flow in
   the terminal and avoids context-switching). Ask the user to confirm the merge
   strategy (`--squash`, `--merge`, or `--rebase`), then run:

   ```bash
   gh pr merge <PR-NUMBER> --<strategy> --delete-branch
   ```

   `--delete-branch` removes the remote branch after a successful merge. If
   required checks are still running, add `--auto` so the merge fires once they
   pass; otherwise omit it for an immediate merge. Stop and surface any merge
   error (conflicts, failing required checks) — do not retry blindly.

   **CI merge gate.** Branch protection on `main` requires every gating CI
   check to be green and the branch to be up-to-date with `main` before a
   merge is allowed (linear history is also enforced — only `--squash` or
   `--rebase` work). If `gh pr merge` reports "required statuses must
   pass" or similar, **fail loudly**: print the failing check names and
   stop. Do **not** wait silently or poll the gate from this skill — the
   user needs to see which check is red and decide how to resolve it
   (fix and push, or open a separate task). The required-checks list and
   the recipe for changing it live in
   [DEVELOPMENT_SETUP.md](../../../docs/developers/DEVELOPMENT_SETUP.md#ci-merge-gate-branch-protection-on-main).

   Note: `--merge` (true merge commit) is rejected by the linear-history
   rule. If the user picks it, surface the constraint and ask them to
   choose `--squash` (the project default) or `--rebase` instead.

   Only fall back to "merge in the GitHub UI" if the user explicitly asks for a
   manual review pass.

### Phase 4 — Release on main

1. **Switch to main and pull**:

    ```bash
    git checkout main
    git pull
    ```

2. **Bump version**: update **both** files atomically:
    - `version =` line in `platformio.ini`
    - `#define FIRMWARE_VERSION` line in `include/version.h`

    Both must show the same `vX.Y.Z` string.

3. **Archive closed tasks**: move every flat `.md` file in `docs/developers/tasks/closed/`
    into `docs/developers/tasks/archive/vX.Y.Z/` using `git mv`.

    ```bash
    mkdir -p docs/developers/tasks/archive/vX.Y.Z
    for f in docs/developers/tasks/closed/*.md; do
      git mv "$f" docs/developers/tasks/archive/vX.Y.Z/
    done
    ```

4. **Regenerate task overview** (regenerates OVERVIEW.md, EPICS.md, and KANBAN.md):

    ```bash
    python scripts/housekeep.py --apply
    git add docs/developers/tasks/
    ```

    Then snapshot the three overviews into `archive/vX.Y.Z/` so the v0.4.0+
    release-snapshot artifacts are produced:

    ```bash
    python scripts/release_snapshot.py vX.Y.Z
    git add docs/developers/tasks/archive/vX.Y.Z/
    ```

5. **Commit the bump and archive**.

    Multi-file release operation — uses the documented bypass:

    ```bash
    git add platformio.ini include/version.h
    ASP_COMMIT_BYPASS="release-branch: version bump + archive" \
        git commit -m "chore: bump version to vX.Y.Z and archive closed tasks"
    ```

6. **Create annotated tag**:

    ```bash
    git tag -a vX.Y.Z -m "Release vX.Y.Z"
    ```

7. **Show a final summary** of what will be pushed (commit + tag), then ask the user to
    confirm before pushing.

8. **Push commit and tag**:

    ```bash
    git push origin main
    git push origin vX.Y.Z
    ```

9. **Remind the user** to create a GitHub Release from the tag in the GitHub UI and
    attach the `.elf`/`.bin` build artefacts for each target environment.

Do not push at any step without explicit user confirmation.
