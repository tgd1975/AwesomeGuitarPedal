# Scripts Documentation

This folder contains utility scripts for development, CI/CD, and maintenance tasks.

## Available Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| [`check_code_smells.py`](check_code_smells.py) | Code smell detector for AwesomeStudioPedal C++ sources. | `python3 check_code_smells.py` |
| [`claude_recap.py`](claude_recap.py) | Run a headless Claude Code recap of recent session transcripts. | `python3 claude_recap.py` |
| [`cleanup-releases.sh`](cleanup-releases.sh) | cleanup-releases.sh — keep only the current + 2 previous GitHub releases. | `./cleanup-releases.sh` |
| [`commit-pathspec.sh`](commit-pathspec.sh) | scripts/commit-pathspec.sh — wrapper for git's pathspec-form commit | `./commit-pathspec.sh` |
| [`format-code.sh`](format-code.sh) | Format all C++ files with clang-format | `./format-code.sh` |
| [`generate-schematic.py`](generate-schematic.py) | Circuit schematic generator for AwesomeStudioPedal. | `python3 generate-schematic.py` |
| [`generate_hid_display_table.py`](generate_hid_display_table.py) | Generate `app/lib/constants/hid_display_table.dart`. | `python3 generate_hid_display_table.py` |
| [`housekeep.py`](housekeep.py) | Central housekeeping for the task system. | `python3 housekeep.py` |
| [`install_git_hooks.sh`](install_git_hooks.sh) | Install repo-side git hooks into .git/hooks/. | `./install_git_hooks.sh` |
| [`migrate_tasks.py`](migrate_tasks.py) | Migrate tasks from REPOSITORY_IMPROVEMENT_CONCEPT.md into | `python3 migrate_tasks.py` |
| [`organize_closed_tasks.py`](organize_closed_tasks.py) | Archive all closed tasks into a versioned release folder under tasks/archive/. | `python3 organize_closed_tasks.py` |
| [`pedal_config.py`](pedal_config.py) | pedal_config.py — CLI tool for scanning, uploading, and validating pedal configs over BLE. | `python3 pedal_config.py` |
| [`pre-commit`](pre-commit) | Comprehensive pre-commit hook for code quality checks | `python3 pre-commit` |
| [`release_burnup.py`](release_burnup.py) | Generate the burn-up section for OVERVIEW.md. | `python3 release_burnup.py` |
| [`release_snapshot.py`](release_snapshot.py) | Snapshot OVERVIEW / EPICS / KANBAN into `archive/<version>/` on release. | `python3 release_snapshot.py` |
| [`screenshot.py`](screenshot.py) | Capture an Android screenshot via adb and resize so the longest edge fits | `python3 screenshot.py` |
| [`security_review_changes.py`](security_review_changes.py) | Security review for incoming changes (pull / merge / rebase). | `python3 security_review_changes.py` |
| [`serial_monitor.py`](serial_monitor.py) | Simple serial monitor for ESP32/Arduino devices | `python3 serial_monitor.py` |
| [`sweep_md025.py`](sweep_md025.py) | TASK-369: Strip redundant body-level H1 from files where frontmatter title is the H1. | `python3 sweep_md025.py` |
| [`sync_skills_config.py`](sync_skills_config.py) | Sync enabled_skills in .vibe/config.toml with .claude/skills/ directory. | `python3 sync_skills_config.py` |
| [`sync_task_system.py`](sync_task_system.py) | Sync the task-system source-of-truth from awesome-task-system/ to live copies. | `python3 sync_task_system.py` |
| [`task_system_config.py`](task_system_config.py) | Shared config loader for the task system. | `python3 task_system_config.py` |
| [`update_idea_overview.py`](update_idea_overview.py) | Regenerate docs/developers/ideas/OVERVIEW.md from idea files in | `python3 update_idea_overview.py` |
| [`update_scripts_readme.py`](update_scripts_readme.py) | Automatically update scripts/README.md based on current scripts in the folder. | `python3 update_scripts_readme.py` |
| [`update_task_overview.py`](update_task_overview.py) | DEPRECATED — prefer `scripts/housekeep.py` for the full flow (file | `python3 update_task_overview.py` |
| [`validate_mermaid.py`](validate_mermaid.py) | Mermaid Diagram Validation Script | `python3 validate_mermaid.py` |
| [`validate_platformio_ini.py`](validate_platformio_ini.py) | Lint platformio.ini. | `python3 validate_platformio_ini.py` |

## Script Details

### check_code_smells.py

**Purpose**: Code smell detector for AwesomeStudioPedal C++ sources.

**Usage**: `python3 check_code_smells.py`

### claude_recap.py

**Purpose**: Run a headless Claude Code recap of recent session transcripts.

**Usage**: `python3 claude_recap.py`

### cleanup-releases.sh

**Purpose**: cleanup-releases.sh — keep only the current + 2 previous GitHub releases.

**Usage**: `./cleanup-releases.sh`

### commit-pathspec.sh

**Purpose**: scripts/commit-pathspec.sh — wrapper for git's pathspec-form commit

**Usage**: `./commit-pathspec.sh`

### format-code.sh

**Purpose**: Format all C++ files with clang-format

**Usage**: `./format-code.sh`

### generate-schematic.py

**Purpose**: Circuit schematic generator for AwesomeStudioPedal.

**Usage**: `python3 generate-schematic.py`

### generate_hid_display_table.py

**Purpose**: Generate `app/lib/constants/hid_display_table.dart`.

**Usage**: `python3 generate_hid_display_table.py`

### housekeep.py

**Purpose**: Central housekeeping for the task system.

**Usage**: `python3 housekeep.py`

### install_git_hooks.sh

**Purpose**: Install repo-side git hooks into .git/hooks/.

**Usage**: `./install_git_hooks.sh`

### migrate_tasks.py

**Purpose**: Migrate tasks from REPOSITORY_IMPROVEMENT_CONCEPT.md into

**Usage**: `python3 migrate_tasks.py`

### organize_closed_tasks.py

**Purpose**: Archive all closed tasks into a versioned release folder under tasks/archive/.

**Usage**: `python3 organize_closed_tasks.py`

### pedal_config.py

**Purpose**: pedal_config.py — CLI tool for scanning, uploading, and validating pedal configs over BLE.

**Usage**: `python3 pedal_config.py`

### pre-commit

**Purpose**: Comprehensive pre-commit hook for code quality checks

**Usage**: `python3 pre-commit`

### release_burnup.py

**Purpose**: Generate the burn-up section for OVERVIEW.md.

**Usage**: `python3 release_burnup.py`

### release_snapshot.py

**Purpose**: Snapshot OVERVIEW / EPICS / KANBAN into `archive/<version>/` on release.

**Usage**: `python3 release_snapshot.py`

### screenshot.py

**Purpose**: Capture an Android screenshot via adb and resize so the longest edge fits

**Usage**: `python3 screenshot.py`

### security_review_changes.py

**Purpose**: Security review for incoming changes (pull / merge / rebase).

**Usage**: `python3 security_review_changes.py`

### serial_monitor.py

**Purpose**: Simple serial monitor for ESP32/Arduino devices

**Usage**: `python3 serial_monitor.py`

### sweep_md025.py

**Purpose**: TASK-369: Strip redundant body-level H1 from files where frontmatter title is the H1.

**Usage**: `python3 sweep_md025.py`

### sync_skills_config.py

**Purpose**: Sync enabled_skills in .vibe/config.toml with .claude/skills/ directory.

**Usage**: `python3 sync_skills_config.py`

### sync_task_system.py

**Purpose**: Sync the task-system source-of-truth from awesome-task-system/ to live copies.

**Usage**: `python3 sync_task_system.py`

### task_system_config.py

**Purpose**: Shared config loader for the task system.

**Usage**: `python3 task_system_config.py`

### update_idea_overview.py

**Purpose**: Regenerate docs/developers/ideas/OVERVIEW.md from idea files in

**Usage**: `python3 update_idea_overview.py`

### update_scripts_readme.py

**Purpose**: Automatically update scripts/README.md based on current scripts in the folder.

**Usage**: `python3 update_scripts_readme.py`

### update_task_overview.py

**Purpose**: DEPRECATED — prefer `scripts/housekeep.py` for the full flow (file

**Usage**: `python3 update_task_overview.py`

### validate_mermaid.py

**Purpose**: Mermaid Diagram Validation Script

**Usage**: `python3 validate_mermaid.py`

### validate_platformio_ini.py

**Purpose**: Lint platformio.ini.

**Usage**: `python3 validate_platformio_ini.py`
