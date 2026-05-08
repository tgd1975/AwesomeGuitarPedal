---
id: IDEA-020
title: MBSE System Documentation with SysML v2
category: docs
description: Retroactively document the system using SysML v2 textual notation — stakeholder needs, derived requirements, structural architecture, behavioural models, and interface contracts — all as committed text files with CI validation and rendered output.
---

## Context and Motivation

The AwesomeStudioPedal firmware is well-engineered: clean interface segregation
(`IBleKeyboard`, `IButtonController`, `ILEDController`), hardware abstraction layers, a
testable pure-logic core, strategy-pattern action hierarchy, and profile-based configuration
via JSON. The architecture is described informally in `ARCHITECTURE.md` and in code-level
Doxygen comments.

What is missing is a **system-level perspective**: the view that sits *above* the code and
answers questions like:

- What is the system *required* to do, and where do those requirements come from?
- What are the system boundaries and external interfaces?
- What are the failure modes the design must handle?
- How does the system behave across its full operational lifecycle?

This is the territory of **Model-Based Systems Engineering (MBSE)** and, specifically for a
docs-as-code project, **SysML v2 textual notation**.

Since the design is already done, this is *retroactive documentation* — an exercise in
formally capturing what has been built, deriving a requirement set from the implemented
behaviour, and establishing a model that future contributors (especially those touching
hardware variants or new features) can extend and trace against.

---

## Why SysML v2 Specifically

SysML v1 (the UML profile) is inherently graphical — it lives in proprietary tools like
Cameo, Enterprise Architect, or MagicDraw. Exporting to text produces unreadable XMI. This
makes it incompatible with docs-as-code.

**SysML v2 is different.** The OMG specification defines a first-class textual notation as
the primary language, with graphical views as *derived* output. A SysML v2 model is a set
of `.sysml` text files:

```sysml
package 'AwesomeStudioPedal' {

    requirement def 'Hands-Free Operation' {
        doc /* A performer shall be able to trigger device actions without
             removing hands from their instrument */
        attribute id : String = "SN-001";
    }

    part def PedalController {
        part profileManager : ProfileManager;
        part eventDispatcher : EventDispatcher;
        // ...
    }
}
```

These files:

- Live in the repository alongside the code.
- Produce readable, reviewable diffs when the model changes.
- Can be validated by CI using the SysML v2 Pilot Implementation.
- Can be rendered to diagrams by the same toolchain.

The textual notation covers the full SysML v2 feature set: requirements, use cases, parts
(blocks), ports, flows, state machines, activities, and allocation relationships. Nothing
requires a GUI to author or review.

---

## Toolchain and CI Landscape

The SysML v2 ecosystem is actively maturing. The honest state as of 2026:

### SysML v2 Pilot Implementation (OMG reference)

The OMG maintains a reference implementation at
`github.com/Systems-Modeling/SysML-v2-Release`. It includes:

- A **Jupyter kernel** (`syside`) that executes `.sysml` cells and renders diagrams in
  notebooks.
- A **REST API server** for storing and querying model elements.
- A **VS Code extension** (`sysml-2-extension`) with syntax highlighting, error checking,
  and live diagram preview.
- A **Docker image** of the API server, suitable for CI.

**CI integration path**: run the Docker image in CI, submit `.sysml` files to the API for
validation, and use the Jupyter kernel (`nbconvert`) to render notebooks as HTML pages that
land in the GitHub Pages deployment.

### syside / Eclipse Sirius (IDEs)

`syside` is the VS Code language server backend. It provides real-time syntax validation and
can be run headlessly for CI linting without the full Docker stack.

### PlantUML (approximation fallback)

PlantUML has SysML v1 block diagram support. It cannot render SysML v2 natively, but it can
render **approximations** of the structural and requirement views that are close enough for
documentation purposes. These PlantUML files (`.puml`) would be generated *from* the `.sysml`
source by a conversion script, not written by hand.

The rendered PNGs/SVGs land in `model/rendered/` and are included in the docs. They are
always stale by definition — the `.sysml` files are the truth.

### Recommended Toolchain for This Project

| Role | Tool | Notes |
|---|---|---|
| Authoring | VS Code + `sysml-2-extension` | Real-time validation and diagram preview |
| Source of truth | `.sysml` text files in `model/` | Committed to the repo |
| CI validation | `syside` headless or Docker API | Syntax and semantic checks |
| Rendering (rich) | Jupyter notebooks + `nbconvert` | HTML pages for GitHub Pages |
| Rendering (fallback) | PlantUML approximation script | SVGs for direct GitHub preview |

---

## Model Scope

The model is divided into five packages, each a subdirectory of `model/`. The scope is
deliberately bounded to what the existing codebase already implements — no speculative
features unless explicitly tagged `«future»`.

### 1. Stakeholder Needs (`model/requirements/stakeholder_needs.sysml`)

The starting point of any requirements hierarchy. These are the *problems the system exists
to solve*, written from the perspective of each persona, not the solution.

```sysml
package 'Stakeholder Needs' {

    requirement def 'SN-001 Hands-Free Foot Control' {
        doc /* A performing musician shall be able to trigger device actions
             using only their feet, without removing hands from their instrument. */
        attribute id : String = "SN-001";
        attribute source : String = "Musician";
    }

    requirement def 'SN-002 Zero-Configuration Pairing' {
        doc /* The device shall work on any Bluetooth-capable host without
             installing drivers, vendor applications, or system extensions. */
        attribute id : String = "SN-002";
        attribute source : String = "Musician";
    }

    requirement def 'SN-003 Remappable Without Recompiling' {
        doc /* A builder shall be able to change all button-to-action mappings
             without modifying or recompiling the firmware. */
        attribute id : String = "SN-003";
        attribute source : String = "Builder";
    }

    requirement def 'SN-004 Multiple Independent Configurations' {
        doc /* A musician shall be able to switch between at least 3 independent
             button-mapping configurations without re-pairing or rebooting the device. */
        attribute id : String = "SN-004";
        attribute source : String = "Musician";
    }

    requirement def 'SN-005 Hardware-Independent Logic' {
        doc /* A developer shall be able to add a new hardware target without
             modifying the core button-handling or profile logic. */
        attribute id : String = "SN-005";
        attribute source : String = "Developer";
    }

    requirement def 'SN-006 Active Profile Visual Feedback' {
        doc /* A musician shall be able to identify the active profile at a glance
             in low-light conditions. */
        attribute id : String = "SN-006";
        attribute source : String = "Musician";
    }

    requirement def 'SN-007 Delayed Action Execution' {
        doc /* A solo performer shall be able to trigger an action (e.g. camera shutter)
             and move into position before the action fires. */
        attribute id : String = "SN-007";
        attribute source : String = "Musician";
    }
}
```

### 2. System Requirements (`model/requirements/system_requirements.sysml`)

Derived from the stakeholder needs. These are the system-level *shalls* — what the device
as a black box must satisfy. Every system requirement traces to at least one stakeholder need.

```sysml
package 'System Requirements' {

    import 'Stakeholder Needs'::*;

    requirement def 'SYS-001 BLE HID Keyboard Profile' {
        doc /* The device shall implement the Bluetooth Low Energy HID
             keyboard profile (HID-over-GATT). */
        attribute id : String = "SYS-001";
        satisfy SN-002;
    }

    requirement def 'SYS-002 Driverless Cross-Platform Compatibility' {
        doc /* The device shall be accepted as a HID keyboard by macOS, iOS,
             Android, and Windows without additional drivers. */
        attribute id : String = "SYS-002";
        satisfy SN-002;
        refine 'SYS-001 BLE HID Keyboard Profile';
    }

    requirement def 'SYS-003 Configurable Button-to-Action Mapping' {
        doc /* All button-to-action assignments shall be stored in a human-editable
             configuration file on the device; no firmware recompilation shall be
             required to change them. */
        attribute id : String = "SYS-003";
        satisfy SN-003;
    }

    requirement def 'SYS-004 Multi-Profile Support' {
        doc /* The device shall support between 1 and 7 independently configurable
             profiles, selectable at runtime without interrupting the BLE connection. */
        attribute id : String = "SYS-004";
        satisfy SN-004;
    }

    requirement def 'SYS-005 Profile Indicator' {
        doc /* The device shall provide unambiguous visual indication of the currently
             active profile using an LED array visible in low-light conditions. */
        attribute id : String = "SYS-005";
        satisfy SN-006;
    }

    requirement def 'SYS-006 Delayed Action' {
        doc /* The device shall support configurable per-button action delay of 0–N
             milliseconds between button press and action execution. */
        attribute id : String = "SYS-006";
        satisfy SN-007;
    }

    requirement def 'SYS-007 Non-Volatile Configuration Persistence' {
        doc /* The active configuration shall persist across power cycles without
             requiring re-upload from the host. */
        attribute id : String = "SYS-007";
        satisfy SN-003;
    }

    requirement def 'SYS-008 Hardware Abstraction' {
        doc /* The device logic shall be hardware-independent and buildable on a
             development host without access to physical hardware. */
        attribute id : String = "SYS-008";
        satisfy SN-005;
    }

    requirement def 'SYS-009 Configuration Recovery' {
        doc /* If the stored configuration is missing or invalid, the device shall
             fall back to a hardcoded default configuration and provide a visual
             error indication. */
        attribute id : String = "SYS-009";
        satisfy SN-002;  // device must still work even with a corrupt config
    }
}
```

### 3. Structural Architecture (`model/architecture/`)

**System context** — the boundary between the device and the world:

```sysml
package 'System Context' {

    part def PedalController;
    part def HostDevice;

    // The system-of-interest and its environment
    part pedal : PedalController {
        port blePort : ~BleHidPort;
        port usbPort : ~UsbPort;
    }

    part host : HostDevice {
        port blePort : BleHidPort;
    }

    connect pedal::blePort to host::blePort;
}
```

**Internal block decomposition** — the components already implemented:

```sysml
package 'Structural Architecture' {

    part def PedalController {
        // Logic layer (hardware-independent)
        part eventDispatcher    : EventDispatcher;
        part profileManager     : ProfileManager;
        part configLoader       : ConfigLoader;
        part actionHierarchy    : ActionHierarchy;

        // Hardware adapters (platform-specific)
        part bleAdapter         : BleKeyboardAdapter;
        part buttonController   : ButtonController;
        part ledController      : LEDController;

        // Storage
        part littleFS           : LittleFSStorage;

        // Internal connections (mirrors ARCHITECTURE.md)
        connect buttonController.eventOut   to eventDispatcher.eventIn;
        connect eventDispatcher.actionOut   to profileManager.actionIn;
        connect profileManager.bleOut       to bleAdapter.bleIn;
        connect profileManager.ledOut       to ledController.ledIn;
        connect configLoader.configOut      to profileManager.configIn;
        connect littleFS.fsOut              to configLoader.fsIn;
    }

    // Hardware abstraction seam (from ARCHITECTURE.md)
    interface def IBleKeyboard;
    interface def IButtonController;
    interface def ILEDController;
}
```

### 4. Behavioural Models (`model/behavior/`)

**Device state machine** — the operational lifecycle:

```sysml
package 'Device Behaviour' {

    state def DeviceLifecycle {
        entry; then Initialising;

        state Initialising {
            entry action loadConfig : 'Load JSON from LittleFS';
            then do action buildActionGraph : 'Instantiate Action objects';
            then BleAdvertising;

            // Fallback path (SYS-009)
            transition on 'Config invalid or missing'
                do action loadDefaults : 'Apply hardcoded defaults'
                then BleAdvertising;
        }

        state BleAdvertising {
            entry action startAdvert : 'Begin BLE advertisement';
            then do action blinkBle : 'BLE LED slow blink';
            transition on 'Host paired' then Connected;
        }

        state Connected {
            entry action solidBle : 'BLE LED solid';
            entry action showProfile : 'Update profile LEDs';

            state Idle;
            state ButtonPressed {
                entry action dispatch : 'EventDispatcher fires action';
            }
            state DelayedActionPending {
                entry action blinkPwr : 'Power LED blinks during countdown';
                transition on 'Delay elapsed' then Idle;
            }
            state ProfileSwitching {
                entry action cycleProfile : 'ProfileManager advances profile';
                entry action updateLeds : 'LEDController shows new profile';
                then Idle;
            }

            transition Idle -> ButtonPressed    on 'Button A–Z pressed';
            transition ButtonPressed -> Idle    on 'Action complete';
            transition ButtonPressed -> DelayedActionPending
                                                on 'DelayedAction started';
            transition Idle -> ProfileSwitching on 'SELECT pressed';
            transition on 'BLE disconnected'    then BleAdvertising;
        }
    }
}
```

**Button-press activity** — the data flow from physical press to BLE report:

```sysml
action def ButtonPressActivity {
    in pin : GpioPin;
    out bleReport : HidReport;

    action debounce        : 'ButtonController: hardware debounce';
    action dispatchEvent   : 'EventDispatcher: route to active profile';
    action resolveAction   : 'ProfileManager: look up Action for button + profile';
    action executeAction   : 'Action: send HID key / string / media key / serial';
    action sendBle         : 'BleKeyboardAdapter: write HID report over GATT';

    flow pin           to debounce;
    flow debounce      to dispatchEvent;
    flow dispatchEvent to resolveAction;
    flow resolveAction to executeAction;
    flow executeAction to sendBle;
    flow sendBle       to bleReport;
}
```

### 5. Interface Contracts (`model/interfaces/`)

Formal definitions of the three hardware-abstraction interfaces — already exist in C++ headers
but benefit from language-neutral SysML representation:

```sysml
package 'Interfaces' {

    interface def IBleKeyboard {
        doc /* Implemented by BleKeyboardAdapter; consumed by Action hierarchy */
        out action press   (key : HidKeyCode);
        out action release (key : HidKeyCode);
        out action print   (text : String);
        out action mediaKey(key : HidMediaKey);
        attribute isConnected : Boolean;
    }

    interface def IButtonController {
        doc /* Implemented by ButtonController; consumed by EventDispatcher */
        out flow buttonEvent : ButtonEvent;  // fires on press, release
    }

    interface def ILEDController {
        doc /* Implemented by LEDController; consumed by ProfileManager */
        out action set   (pin : LedPin; state : Boolean);
        out action blink (pin : LedPin; intervalMs : Integer);
    }
}
```

---

## Repository Structure

```
model/
├── README.md                               ← how to open, edit, and render the model
├── AwesomeStudioPedal.sysml               ← top-level package; imports all sub-packages
│
├── requirements/
│   ├── stakeholder_needs.sysml
│   ├── system_requirements.sysml
│   ├── software_requirements.sysml        ← derived from SYS-* for the firmware
│   └── hardware_requirements.sysml        ← derived from SYS-* for the electronics
│
├── architecture/
│   ├── system_context.sysml
│   ├── structural.sysml
│   └── interfaces.sysml
│
├── behavior/
│   ├── use_cases.sysml
│   ├── state_machine.sysml
│   └── activities.sysml
│
├── notebooks/
│   ├── system_overview.ipynb              ← rendered model overview (Jupyter + SysML v2 kernel)
│   └── requirements_traceability.ipynb   ← traceability matrix: SN → SYS → SW/HW
│
└── rendered/                              ← CI-generated, do not edit by hand
    ├── requirements_tree.svg
    ├── structural_ibd.svg
    └── state_machine.svg
```

The `rendered/` directory is committed so that GitHub renders the SVGs in the repository
browser without needing to run the toolchain locally. CI regenerates it on every push that
touches `model/`.

---

## CI Integration

A new workflow `model.yml` runs on pushes to `main` and on PRs that touch `model/`:

```yaml
jobs:
  validate-model:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      # Option A: syside headless for syntax checking (lighter, faster)
      - name: Install syside
        run: npm install -g @sensmetry/syside-cli

      - name: Validate SysML v2 files
        run: syside validate model/

      # Option B: SysML v2 Pilot Implementation (heavier, full semantics)
      # Uncomment when the project needs full semantic validation
      # - name: Start SysML v2 API
      #   run: docker run -d -p 9000:9000 sysml2/api:latest
      # - name: Submit model to API
      #   run: python scripts/submit_sysml_model.py model/

  render-model:
    runs-on: ubuntu-latest
    needs: validate-model
    if: github.event_name != 'pull_request'
    steps:
      - uses: actions/checkout@v4

      - name: Install Jupyter + SysML v2 kernel
        run: |
          pip install jupyter nbconvert
          pip install sysml2-jupyter-kernel  # or equivalent

      - name: Render notebooks to HTML
        run: |
          jupyter nbconvert --to html model/notebooks/*.ipynb \
            --output-dir docs/api/html/model/

      - name: Generate PlantUML approximations
        run: python scripts/sysml_to_plantuml.py model/ model/rendered/

      # Rendered output is published by the existing docs.yml → GitHub Pages
```

The rendered HTML notebooks become a section of the existing GitHub Pages site:
`https://tgd1975.github.io/AwesomeStudioPedal/model/`.

---

## Traceability Strategy

The requirement IDs (`SN-001`, `SYS-001`, etc.) form the traceability backbone. The model
makes two types of trace explicit:

1. **`satisfy` relationships** — a system requirement satisfies one or more stakeholder needs.
2. **`refine` relationships** — a lower-level requirement refines a higher-level one.

Additionally, source code can reference requirement IDs in comments:

```cpp
// Satisfies: SYS-003 (Configurable Button-to-Action Mapping)
// Satisfies: SYS-007 (Non-Volatile Configuration Persistence)
bool ConfigLoader::loadFromFilesystem() { ... }
```

A simple CI script can scan `.cpp`/`.h` files for `Satisfies: SYS-NNN` annotations and
generate a bidirectional traceability matrix:

```
SYS-003 ← config_loader.cpp, profile_manager.cpp
SYS-008 ← arduino_shim.h (test), i_ble_keyboard.h, i_button_controller.h, i_led_controller.h
```

This matrix is rendered in the `requirements_traceability.ipynb` notebook and published on
GitHub Pages. It does not require an external MBSE tool — the model files and a Python script
are sufficient.

---

## Relationship to Other Ideas

- **IDEA-019 (Wiring as Code)**: The hardware requirements in `model/requirements/hardware_requirements.sysml`
  formally derive from the system requirements. When IDEA-019's Schemdraw schematic is
  implemented, the schematic should trace back to `HW-REQ-*` identifiers using the same
  source-comment convention.
- **IDEA-018 (BOM)**: The BOM line items are the physical realisations of hardware
  requirements. `HW-REQ-001: The device shall include a BLE-capable microcontroller` →
  BOM line 1: `ESP32 NodeMCU-32S`.
- **IDEA-011 (PCB Design)**: The structural model and interface contracts define what the PCB
  must implement. When IDEA-011 starts, the SysML model provides the system-level
  specification that the PCB design must satisfy.
- **IDEA-003 / IDEA-004 (Additional Hardware Support)**: Adding a new hardware target means
  extending the `model/architecture/structural.sysml` with a new platform block and verifying
  that it satisfies the same `SYS-*` requirements as the ESP32 target. The model makes this
  evaluation systematic rather than implicit.

---

## Open Questions

| # | Question | Impact |
|---|---|---|
| Q1 | Which CI tool for SysML v2 validation — `syside` headless (fast, lighter semantics) or the Docker API (slower, full OMG semantics)? | Determines CI runtime and setup complexity |
| Q2 | Should the software requirements (`SW-REQ-*`) be derived manually or generated from the Doxygen-annotated interfaces? | A generator would keep them in sync but adds tooling complexity |
| Q3 | Should requirement IDs appear in C++ source comments (for traceability) or is the model-level `satisfy` relationship sufficient? | Code-level annotations are powerful but add maintenance discipline |
| Q4 | Are the notebooks the primary rendered output, or is a separate SysML viewer (e.g. Papyrus web) worth evaluating? | Affects the GitHub Pages structure |
| Q5 | Should the model include a parametric diagram for the binary LED encoding table currently in `HARDWARE_CONFIG.md`? | This is exactly the kind of constraint SysML parametrics were designed for; probably yes |
