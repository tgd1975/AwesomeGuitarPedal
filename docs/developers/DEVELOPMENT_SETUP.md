# Development Setup

## Required Tools

> **Quickest setup:** open the project in a [VS Code Dev Container](https://code.visualstudio.com/docs/devcontainers/containers)
> or [GitHub Codespaces](https://github.com/features/codespaces) — all tools below are installed
> automatically via [.devcontainer/devcontainer.json](../../.devcontainer/devcontainer.json).

## Dev Container (local) and GitHub Codespaces

**Local dev container:** open the project folder in VS Code and accept the
"Reopen in Container" prompt, or run **Dev Containers: Reopen in Container** from the
command palette. Requires Docker and the
[Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers).

**Codespaces:** click **Code → Codespaces → Create codespace on `main`** (or your branch)
on the GitHub repo page.

Both environments start with all tools pre-installed and the CMake build directory configured.

**Works everywhere:**

- `make test-host` — host unit tests via GoogleTest (primary workflow)
- `make build-esp32` / `make build-nrf52840` — firmware compilation
- All quality tools: `/clang-tidy`, `/lint`, `/format`, `/fix-markdown`
- `/ci-status`, editing, reviewing, opening PRs

**Does not work in Codespaces (no USB):**

- `make upload-*` / `make run-*` — flashing requires a physical device connected via USB
- `make monitor-*` — serial monitor requires a physical device
- `make test-esp32-*` / `make test-nrf52840-*` — on-device tests require hardware

**PlatformIO note:** the PlatformIO CLI (`pio`) is installed once during container creation
(`postCreateCommand`). The first `pio run` downloads the ESP32/nRF52840 toolchains (~300 MB)
and caches them for the life of the container.

### USB / serial device access

The devcontainer runs with `"privileged": true`, which grants access to all USB devices
that are present on the host when the container starts. When no device is connected the
container starts normally — flash and monitor commands simply report "device not found",
which is the expected behaviour in Codespaces and CI.

#### Linux host

Connect the NodeMCU-32S before starting (or restarting) the container. The CP210x
USB-to-serial chip appears as `/dev/ttyUSB0` (first device) or `/dev/ttyUSB1` if another
serial device is already connected.

```bash
# Verify the device is visible inside the container:
ls /dev/ttyUSB*
```

PlatformIO auto-detects the port. If it picks the wrong one, override it in
`platformio.ini`:

```ini
upload_port = /dev/ttyUSB0
monitor_port = /dev/ttyUSB0
```

If you get `Permission denied` even with `--privileged`, your host user may not be in the
`dialout` group. Fix on the host (not the container):

```bash
sudo usermod -aG dialout $USER   # log out and back in to take effect
```

#### Windows host (Docker Desktop + WSL2)

Docker Desktop on Windows uses a WSL2 backend. USB devices are on the Windows side and
must be forwarded into WSL2 before the container can see them. Use
[**usbipd-win**](https://github.com/dorssel/usbipd-win):

1. Install usbipd-win (once):

   ```powershell
   winget install usbipd
   ```

2. List USB devices and find the NodeMCU-32S (CP210x Silicon Labs):

   ```powershell
   usbipd list
   # Look for: "Silicon Labs CP210x USB to UART Bridge"
   # Note its BUSID, e.g. 3-1
   ```

3. Attach it to WSL2 (run in an **Administrator** PowerShell):

   ```powershell
   usbipd attach --wsl --busid 3-1
   ```

4. Start (or rebuild) the dev container. The device appears as `/dev/ttyUSB0` inside the
   container exactly as on Linux.

5. To detach when done:

   ```powershell
   usbipd detach --busid 3-1
   ```

> The `usbipd attach` step must be repeated every time the device is unplugged or the
> machine is rebooted. Consider pinning the command to a terminal profile for convenience.

#### GitHub Codespaces

No USB access. All hardware-dependent `make` targets will fail with a clear error. This is
expected — use Codespaces for firmware builds, host tests, linting, and code review only.

### Host folder mounts (local dev container only)

The following folders are bind-mounted from the host into the container so that AI tool
credentials are available without storing any secrets in the repository:

| Host path | Container path | Purpose |
|-----------|---------------|---------|
| `~/.claude` | `/home/vscode/.claude` | Claude Code authentication |

These mounts are transparent — credentials set up on the host are used automatically inside
the container. Nothing is copied into the image or committed to the repo.

> **Codespaces:** the mounts above are host-only and do not apply. Authenticate Claude Code
> and Mistral Vibe manually after the codespace starts, or pass `ANTHROPIC_API_KEY` /
> `MISTRAL_API_KEY` as Codespaces secrets.

### Dev container secrets (API keys)

Secrets are passed into the container via `.devcontainer/.env` — a file that lives on your
machine but is **never committed** (it is in `.gitignore`).

On a fresh clone, `initializeCommand` automatically copies `.devcontainer/.env.example` to
`.devcontainer/.env` so Docker never fails on a missing file. Open the copy and fill in your
values:

```
# .devcontainer/.env
MISTRAL_API_KEY=your-key-here
```

The container picks up every key in that file on startup via `runArgs --env-file`. After
editing `.env`, **rebuild the container** (`Dev Containers: Rebuild Container`) for the new
values to take effect.

To add a new secret:

1. Add the key name (no value) to `.devcontainer/.env.example` with a comment — commit this.
2. Add the key and value to your local `.devcontainer/.env` — do **not** commit this.
3. Reference `${ENV_VAR_NAME}` wherever needed (e.g. `postCreateCommand`, extension config).

| Key | Used by |
|-----|---------|
| `MISTRAL_API_KEY` | `mistral-vibe` VS Code extension + `vibe` CLI |

> **Codespaces:** `.env` file mounts do not work in Codespaces. Pass secrets as
> [Codespaces encrypted secrets](https://docs.github.com/en/codespaces/managing-your-codespaces/managing-your-account-specific-secrets-for-github-codespaces)
> in your GitHub account settings instead.

---

### All platforms

| Tool | Min version | Required for | Install |
|------|-------------|--------------|---------|
| [PlatformIO](https://platformio.org/install/cli) | any recent | Build + flash firmware | `pip install platformio` or VS Code extension |
| [CMake](https://cmake.org/download/) | 3.20 | Host unit tests | package manager or cmake.org |
| [Ninja](https://ninja-build.org/) | any | Host unit tests | `apt install ninja-build` / `brew install ninja` |
| [GCC / G++](https://gcc.gnu.org/) | 11+ | Host unit tests (Linux/macOS) | `apt install g++` / Xcode CLT |
| [clang-format](https://clang.llvm.org/docs/ClangFormat.html) | 14+ | C++ formatting (pre-commit hook) | `apt install clang-format` / `brew install llvm` |
| [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) | 14+ | Static analysis | `apt install clang-tidy` / `brew install llvm` |
| [Python 3](https://www.python.org/downloads/) | 3.9+ | Scripts, PlatformIO | system or python.org |
| [Node.js](https://nodejs.org/) | 20+ | markdownlint, Mermaid CLI | `apt install nodejs` / nodejs.org |
| [Git](https://git-scm.com/) | any | Version control | `apt install git` / Xcode CLT |
| [GitHub CLI (`gh`)](https://cli.github.com/) | any | `/ci-status`, `/pr` skills | `apt install gh` / `brew install gh` |

GoogleTest is fetched automatically by CMake (v1.15.2 via `FetchContent`) — no manual install needed.

### ESP32 flashing

| Tool | Notes | Install |
|------|-------|---------|
| **esptool** | Bundled with PlatformIO — no separate install needed | — |
| **USB-serial driver** | NodeMCU-32S uses CP2102 (CP210x). Linux: built-in kernel module. macOS: [Silicon Labs driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers). Windows: same Silicon Labs driver. | See link |
| **Serial port access (Linux)** | Add your user to the `dialout` group: `sudo usermod -aG dialout $USER`, then log out and back in. See [FLASHING.md](../builders/FLASHING.md). | — |

### nRF52840 flashing

> nRF52840 support is implemented but untested. Use at your own risk.

| Tool | Notes | Install |
|------|-------|---------|
| [nRF Command Line Tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools) | `nrfjprog` for flashing via J-Link | nordicsemi.com |
| [J-Link software](https://www.segger.com/downloads/jlink/) | Required for SWD programming | segger.com |
| **USB driver** | Adafruit Feather nRF52840 uses native USB. Windows may need [Adafruit's driver](https://learn.adafruit.com/adafruit-feather-m0-express-designed-for-circuit-python-circuitpython/using-with-arduino-ide#windows-driver-installation-3-3). | See link |

### Optional tools

| Tool | Used for |
|------|----------|
| [lcov + genhtml](https://github.com/linux-test-project/lcov) | `make coverage` — HTML coverage report. `apt install lcov`. |
| [Doxygen](https://www.doxygen.nl/download.html) | `make docs` — API documentation. `apt install doxygen`. |
| [direnv](https://direnv.net/) | Per-developer device handles (Pixel serial, ESP32 port, pedal MAC) — see "Project env vars" below. `apt install direnv`. |

---

## Project env vars (direnv)

Per-developer device handles — Pixel serial, ESP32/nRF USB port, BLE pedal MAC — live in a
local `.envrc` so skills and scripts never hard-code them inline. The repo ships
[`.envrc.example`](../../.envrc.example) as the template.

```bash
sudo apt install direnv                # one-time
eval "$(direnv hook bash)"             # add to ~/.bashrc
cp .envrc.example .envrc               # in repo root
$EDITOR .envrc                         # fill in your device values
direnv allow                           # one-time, per repo
```

Whenever your shell is inside the repo, `$ASP_PIXEL_DEVICE`, `$ASP_ESP32_PORT`,
`$ASP_NRF52840_PORT`, and `$ASP_PEDAL_MAC` (plus `$ANDROID_SERIAL` mirrored from
`$ASP_PIXEL_DEVICE`) are exported. `.envrc` is gitignored — each developer keeps their own.

If you skip direnv: source `.envrc` manually before running anything that needs the values
(`source .envrc`), or set the variables some other way. The skills (`/ui-dump`,
`/verify-on-device`, `/ble-reset`) all auto-detect a single connected device when the env
var is unset, so the strict requirement is only when you have multiple devices plugged in.

---

## Git hooks

The repo's pre-commit hook (formatting, schema validation, `platformio.ini`
lint, unit tests, secret scan, task-system regen) lives at
[`scripts/pre-commit`](../../scripts/pre-commit).
The security-review hooks (`pre-merge-commit`, `post-merge`, `pre-rebase`)
live in [`scripts/git-hooks/`](../../scripts/git-hooks/). Install all of them
in one go after cloning:

```bash
bash scripts/install_git_hooks.sh
```

The installer is idempotent and safe to re-run. It:

- symlinks the security-review hooks (or copies them on Windows where symlinks
  fail), and
- writes a tiny wrapper at `.git/hooks/pre-commit` that `exec`s
  `scripts/pre-commit`. The wrapper means edits to `scripts/pre-commit`
  take effect immediately on every platform — no re-install needed.

To bypass the security-review hooks for a single command:
`ASP_SKIP_SECURITY_REVIEW=1 git pull`. The pre-commit hook can be bypassed
with `git commit --no-verify`, but this is strongly discouraged — see the
"Pre-commit hook failures on unrelated changes" section in
[`CLAUDE.md`](../../CLAUDE.md) for the case-by-case decision rules.

### platformio.ini lint

When `platformio.ini` is in the staged pathspec, the pre-commit hook runs
[`scripts/validate_platformio_ini.py`](../../scripts/validate_platformio_ini.py),
which wraps `pio project config` and `pio project config --lint` to catch
syntax errors, bad value types (e.g. non-integer `upload_speed`), and
unknown / typo'd keys (e.g. `boord =` instead of `board =`). The same
script runs in CI as the `Static Analysis / platformio.ini lint` job on
every push and PR.

To reproduce a CI failure locally:

```bash
python scripts/validate_platformio_ini.py
```

The script auto-discovers `pio` in `~/.platformio/penv/bin/pio` if it is
not on `PATH` (the pip-installed PlatformIO ships its own venv).

---

## CI merge gate (branch protection on `main`)

The local pre-commit hook is the first line of defence; the CI merge gate
on `main` is the second. GitHub branch protection on `main` requires every
listed check to report success before the **Merge pull request** button
becomes available, and requires the PR branch to be up to date with
`main` before merging. Linear history is enforced — no merge commits.
This matches the existing `/release-branch` squash-and-merge flow.

### Required checks

The check name in the table is the value GitHub uses, which is the
`name:` of the job inside the workflow file (not the workflow file name).
Renaming a job silently breaks branch protection — see "How to add a new
required check" below.

| Check name | Workflow file | Job |
|---|---|---|
| C++ Format Check | `test.yml` | `cpp-format` |
| Markdown Lint | `test.yml` | `markdown-lint` |
| Mermaid Diagram Lint | `test.yml` | `mermaid-lint` |
| Host Unit Tests | `test.yml` | `unit-tests` |
| Code Coverage | `test.yml` | `coverage` |
| clang-tidy | `static-analysis.yml` | `clang-tidy` |
| platformio.ini lint | `static-analysis.yml` | `platformio-ini` |
| Analyze, test, and build APK | `app.yml` | `build-and-test` |
| Flutter Quality | `flutter.yml` | `flutter-quality` |
| Validate profile files | `profiles.yml` | `validate-profiles` |
| Build API Documentation | `docs.yml` | `build-docs` |
| CodeQL | `codeql-analysis.yml` | `analyze` |

Branch protection also has these settings on `main`:

- Require pull request before merging.
- Require branches to be up to date before merging.
- Require linear history (no merge commits).
- Do not allow administrators to bypass.

### Checks deliberately excluded (for now)

These checks run on every PR but are **not** required gates yet because
they are currently red on `main`. Each is a planned-required gate; the
linked task will turn it green, after which the gate-list above must be
updated and branch protection synced (see "How to add a new required
check").

| Check name | Blocked by |
|---|---|
| Schemdraw staleness guard (`docs.yml › schematic-check`) | TASK-370 (Schemdraw SVGs are non-deterministic) |

`release.yml` jobs (`Build ESP32 firmware`, `Build nRF52840 firmware`,
`Build Android APK`, `Create GitHub Release`) and `Deploy to GitHub
Pages` are excluded by design — they only run on `main` push or tag, so
they do not produce check runs on PRs and cannot gate them.

`Auto-merge Dependabot patch updates` from `dependabot-automerge.yml`
is internal to Dependabot's flow and is not a generic merge gate.

### How to add a new required check

GitHub stores required checks by **name**, not by job ID. The name comes
from the `name:` field of the job inside the workflow YAML. A required
check that has never produced a check run cannot be added — GitHub does
not know it exists yet. The order is therefore:

1. Add the workflow / job in `.github/workflows/<file>.yml`. Make sure
   the job has an explicit `name:` — otherwise GitHub falls back to the
   job ID, which is rarely what you want, and renaming later silently
   breaks the gate.
2. Open a PR with a commit that triggers the new workflow on
   `pull_request`. Wait for the run to finish (success or failure does
   not matter — GitHub registers the check name on first run).
3. Add the new check name to the **Required checks** table in this
   document. Keep the table in alphabetical order of workflow file for
   easy diff.
4. Add the same check name to GitHub branch protection on `main`:

   ```bash
   # See current required checks
   gh api repos/:owner/:repo/branches/main/protection/required_status_checks \
     --jq '.contexts'

   # Replace the full list (additive flag: --add not supported by the API).
   # Edit the JSON to include every existing check + the new one, then:
   gh api -X PATCH repos/:owner/:repo/branches/main/protection/required_status_checks \
     --input required_checks.json
   ```

   The `--input` form expects a JSON file with the full check list:
   `{"strict": true, "contexts": ["Check 1", "Check 2", ...]}`. Always
   round-trip through `--jq '.contexts'` first to avoid dropping a
   check by accident.
5. Open a throwaway PR that triggers a deliberate failure of the new
   check, and confirm the **Merge pull request** button is disabled
   with a "required statuses must pass" message. Close the PR without
   merging.

If a check that previously gated `main` is renamed in its workflow YAML,
the old name stays required (and now never reports), and the new name
is unknown to branch protection. To rename safely:

1. Land the rename on a feature branch and let CI run once with the new
   name.
2. Add the new name to required checks (step 4 above).
3. Remove the old name in the same `--input` JSON.

### Local vs CI

Local pre-commit hooks (`scripts/pre-commit`) and `/lint` cover the same
ground as several gates, but the gate is enforced server-side and is the
final word. A passing `/commit` does **not** guarantee a mergeable PR —
the PR still has to clear the CI gate listed above. See
[`docs/developers/COMMIT_POLICY.md`](COMMIT_POLICY.md) for how the local
hook composes with the gate.

---

## Flutter app development

The companion app lives in `app/`. It requires Flutter (installed at `/opt/flutter` on the
reference Ubuntu machine) and the Android SDK (`/opt/Android/Sdk`).

### Running the app

**On a physical Android device (recommended):**

1. Enable Developer Options: Settings → About Phone → tap **Build number** 7 times.
2. Enable USB Debugging: Settings → System → Developer Options → USB Debugging.
3. Connect via USB and confirm the prompt on the device.
4. Verify Flutter sees it, then run the app:

```bash
flutter devices
flutter run
```

Flutter auto-selects the only connected mobile device. Hot reload (`r`) and hot
restart (`R`) work exactly as with an emulator. This approach uses zero extra RAM
on the development machine and is the preferred workflow.

**In the Android emulator:**

```bash
flutter emulators --launch Lean_API33   # start the lean AVD
flutter run
```

The `Lean_API33` AVD uses the `android-33;default` image (no Google Play Services),
1 GB RAM, 720×1280 display, and 2 vCPUs — tuned to be light on host resources.

**In Chromium (UI-only, no BLE):**

```bash
flutter run -d chrome
```

Chromium is detected automatically via `CHROME_EXECUTABLE` in `.bashrc`. This mode
is only useful for iterating on pure UI screens — the following packages do **not**
work on web:

| Package | Limitation |
|---|---|
| `flutter_blue_plus` | No Web Bluetooth support — all BLE calls fail |
| `file_picker` | No native file system access on web |
| `share_plus` | Not supported on web |
| `path_provider` | Not supported on web |

For any screen that touches BLE or the file system, test on a physical device or emulator.

### Asset path note

Schema files (`profiles.schema.json`, `config.schema.json`) are bundled inside the app
under `app/assets/` and referenced as `assets/*.schema.json` in `pubspec.yaml`. The
source of truth remains `data/` at the repo root — copy updated schemas into `app/assets/`
before building if you change them.

---

## Build command reference

| Command | What it does | Hardware needed |
|---------|--------------|----------------|
| `make build-esp32` | Build firmware for ESP32 | No |
| `make upload-esp32` | Flash firmware to ESP32 | Yes |
| `make uploadfs-esp32` | Upload `data/` filesystem image to ESP32 | Yes |
| `make run-esp32` | Build + upload firmware + upload filesystem + monitor | Yes |
| `make monitor-esp32` | Open serial monitor | Yes |
| `make test-host` | Run host unit tests (GoogleTest) | No |
| `make test-esp32-button` | Upload filesystem + on-device button tests | Yes (ESP32) |
| `make test-esp32-serial` | Upload filesystem + on-device serial tests | Yes (ESP32) |
| `make test-esp32-profilemanager` | Upload filesystem + on-device profile manager tests | Yes (ESP32) |
| `make build-nrf52840` | Build firmware for nRF52840 | No |
| `make upload-nrf52840` | Flash firmware to nRF52840 | Yes |
| `make uploadfs-nrf52840` | Upload `data/` filesystem image to nRF52840 | Yes |
| `make run-nrf52840` | Build + upload firmware + upload filesystem + monitor | Yes (nRF52840) |
| `make test-nrf52840-profilemanager` | Upload filesystem + on-device profile manager tests | Yes (nRF52840) |

## Updating the configuration without recompiling

`data/pedal_config.json` is the only file that needs to change when adding, removing, or editing
profiles and button actions. The firmware itself does not need to be rebuilt — only the filesystem
partition is re-flashed.

```bash
# Edit the config, then:
make uploadfs-esp32      # ESP32
make uploadfs-nrf52840   # nRF52840
```

The device reads the config at startup via `ConfigLoader::loadFromFile()`, so the next boot will
pick up the changes automatically. This is the normal workflow for profile customisation.

All `make test-esp32-*` and `make test-nrf52840-*` targets include the filesystem upload as a
prerequisite, so you never need to run it manually before running device tests.

## Host test build

Host tests run on the development machine via CMake and Ninja:

```bash
cmake -B .vscode/build-host -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build .vscode/build-host --target pedal_tests
```

Or use the Makefile shortcut:

```bash
make test-host
```

## Serial port setup on Linux

See [FLASHING.md](../builders/FLASHING.md) for the `dialout` group setup steps.

## Alternative serial monitor

If PlatformIO's built-in monitor has issues, use the Python script:

```bash
python3 scripts/serial_monitor.py /dev/ttyUSB0 115200
```

## Code coverage (local)

Generate an HTML coverage report locally (requires `lcov` and `genhtml`):

```bash
make coverage
# Output: docs/coverage/index.html
```

The CI enforces a **minimum 80% line coverage** threshold. The `coverage` job in
`test.yml` builds with `--coverage`, runs the test suite, and uses `gcovr` to
generate an HTML + XML report and fail the build if coverage drops below 80%.
The HTML report is uploaded as a `coverage-html` artifact on every CI run.

The `docs/coverage/` directory is gitignored — generated output is never committed.

## API documentation (local)

Generate Doxygen output locally:

```bash
doxygen Doxyfile
# Output: docs/api/html/index.html
```

The CI builds Doxygen on every push and pull request. On pushes to `main` the
output is deployed to GitHub Pages. HTML is also uploaded as an `api-docs-html`
artifact on every CI run.

The `docs/api/` directory is gitignored — generated output is never committed.
