---
id: IDEA-017
title: Community Profiles Repository
category: apps
description: A curated folder of ready-to-use profiles for popular tools and workflows, directly accessible from the web configurators and the mobile app — with a call for community contributions.
---

## The Idea in One Sentence

A `profiles/` folder inside the repository holds ready-made `profiles.json` files for popular
tools and workflows; the web configurators and the mobile app can browse, preview, and load them
in one click — and any community member can contribute their own.

---

## Why This Matters

Right now, every user who picks up AwesomeStudioPedal starts from a blank canvas. They either
hand-write a `profiles.json`, or they adapt `docs/simulator/example.json`, which is a generic
demo file — not a real-world workflow. The gap between "firmware flashed" and "actually useful"
is too wide.

A community profiles repository collapses that gap. A musician opens the Configuration Builder,
clicks "Browse profiles", sees "OBS Stream Deck (4 buttons)", loads it, tweaks one key — done.
A builder flashing their first unit gets a working starting point without reading the
key reference. A developer contributing their first feature sees that the repo already has a
healthy community using it.

The secondary effect is social: every contributed profile is a data point that someone real
built and used this project. That is better marketing than any article.

---

## Repository Structure

```
profiles/
├── index.json                         ← auto-generated manifest (CI updates on each merge)
├── CONTRIBUTING.md                    ← how to submit a profile
│
├── 1-button/                          ← works on a 1-button build
│   ├── shutter-delayed.json           ← 3-second delayed camera shutter
│   └── play-pause-media.json          ← universal media play/pause
│
├── 2-button/                          ← requires at least 2 buttons
│   ├── page-turner-basic.json         ← prev/next page (any sheet music app)
│   ├── daw-transport-minimal.json     ← play + record
│   └── zoom-mute-toggle.json          ← mute + raise-hand for video calls
│
├── 3-button/
│   ├── vlc-practice.json              ← play/pause + speed down/up
│   └── podcast-control.json           ← mute + mark + start/stop recording
│
└── 4-button/                          ← full 4-button build (standard prototype)
    ├── score-navigator.json           ← forScore / MobileSheets / Acrobat
    ├── pixel-camera-remote.json       ← Google Pixel shutter + delayed shutter
    ├── obs-stream-deck.json           ← OBS scene switching + mute
    ├── ableton-looper.json            ← Record, Play, Undo, Metronome
    ├── logic-pro-tracking.json        ← Punch-in, Play, Undo, Count-In
    ├── discord-comms.json             ← Mute, Deafen, Quick message, Screenshot
    └── lightroom-culling.json         ← Flag, Reject, Next, Prev
```

The subfolder name reflects the **minimum number of buttons required**, not the total profile
count. A profile in `2-button/` may define only buttons A and B; a pedal with 4 buttons will
still load and use it fine — the unused buttons simply do nothing.

---

## The `index.json` Manifest

Every tool and the app reads a single machine-readable index. CI regenerates it on every merge
to `main` that touches the `profiles/` folder.

```json
{
  "version": 1,
  "generated": "2026-04-19T00:00:00Z",
  "profiles": [
    {
      "id": "score-navigator",
      "name": "Score Navigator",
      "description": "Page navigation for forScore, MobileSheets, and PDF readers. Uses standard Page Up / Page Down keys.",
      "tags": ["music", "sheet-music", "tablets", "page-turner"],
      "minButtons": 2,
      "profileCount": 1,
      "profiles": ["Score Navigator"],
      "file": "4-button/score-navigator.json",
      "author": "tgd1975",
      "tested": ["forScore (iOS)", "MobileSheets (Android)", "Adobe Acrobat (macOS)"]
    },
    {
      "id": "obs-stream-deck",
      "name": "OBS Stream Deck",
      "description": "Foot-operated scene switcher and mic mute for OBS Studio. Uses F13–F16 to avoid keyboard conflicts.",
      "tags": ["streaming", "obs", "video", "podcasting"],
      "minButtons": 4,
      "profileCount": 1,
      "profiles": ["OBS Stream Deck"],
      "file": "4-button/obs-stream-deck.json",
      "author": "tgd1975",
      "tested": ["OBS Studio 30 (macOS)", "OBS Studio 30 (Windows)"]
    }
  ]
}
```

Key fields:

- `minButtons` — used by the tools to filter the gallery by hardware variant.
- `profileCount` — number of profiles in the set (1–7). Shown in the gallery so users know whether they are loading one mode or a full multi-mode set.
- `profiles` — list of profile names in the set. Lets the gallery preview the contents without fetching the full file.
- `tags` — free-form; used for search/filter in the UI.
- `minFirmware` — optional semver string. Omitting it means "any firmware". Set by the contributor when the profile uses action types that require a specific firmware version. Surfaced as a warning badge in the tools and app if the device's firmware is older.
- `tested` — optional. Shows users what the contributor has actually verified. Honesty over marketing.
- `author` — GitHub username. Links back to the contributor's profile.

---

## Integration Points

### Web Config Builder (`docs/tools/config-builder/`)

Currently has a hardcoded inline example profile and a "Load from file" button. Add:

1. **"Browse community profiles" button** — opens a modal panel.
2. The panel fetches `index.json` from GitHub Pages
   (`https://tgd1975.github.io/AwesomeStudioPedal/profiles/index.json`).
3. Profiles are shown as cards: name, description, tags, `minButtons` badge, author.
4. Optional filter: "Show only profiles for N buttons".
5. Clicking a card fetches the full profile JSON and calls the existing `loadFromJson()`
   function — same code path as the manual file loader. No new parser needed.

### Web Profile Builder / Simulator (`docs/simulator/`)

The simulator already loads `example.json` on startup. That default should be replaced or
supplemented with a "choose a starting point" screen that shows the community gallery.

### Mobile App (`app/`)

The app's `FileService` already handles loading profiles from the local filesystem and from
BLE upload. Add a third source:

- A `CommunityProfilesService` that fetches `index.json` and individual profile files
  from the GitHub Pages URL.
- A "Community Profiles" screen in the app: card list, search by tag, filter by button count,
  preview action names, one-tap load into the editor.
- No authentication required — all reads are public HTTP GET against GitHub Pages.
- The service caches the index and individual files locally for offline use (standard
  Flutter HTTP caching with `http` package + `path_provider`).

### Offline / Air-Gapped Use

The `profiles/` folder is part of the repo, so anyone who clones it has every profile locally.
The tools can optionally accept a `--profiles-path` argument (for the web tools: a local file
picker fallback) for users who prefer not to fetch from the internet.

---

## Terminology: Profile vs. Profile Set

These two terms are used precisely throughout this document:

| Term | Definition |
|---|---|
| **Profile** | One named mode — a single entry in the `"profiles"` array. Defines what each button does in that mode. Example: "OBS Stream Deck". |
| **Profile set** | A complete `profiles.json` file containing 1–7 profiles. This is what the firmware loads and what the tools read and write. |

Every file in the `profiles/` folder is a **profile set** — even if it contains only one profile.
This keeps the file format identical to what the firmware expects, so any community file can be
flashed directly without re-wrapping. A contributor who has three DAW modes they switch between
can contribute them as a single set; a contributor with one use case contributes a set of one.
The most common case is a set of one, but the structure never forces that.

---

## Profile File Format

Each file in `profiles/` is a valid `profiles.json` as defined by `data/profiles.schema.json`.
A one-profile set looks like this:

```json
{
  "_doc": "Key reference: docs/builders/KEY_REFERENCE.md",
  "_meta": {
    "id": "obs-stream-deck",
    "author": "tgd1975",
    "tested": ["OBS Studio 30 (macOS)"],
    "tags": ["streaming", "obs", "video"],
    "minFirmware": "1.0.0"
  },
  "profiles": [
    {
      "name": "OBS Stream Deck",
      "description": "Foot-operated scene switcher for OBS Studio. Assign F13–F16 in OBS hotkey settings.",
      "buttons": {
        "A": { "type": "SendCharAction", "name": "Intro Scene",  "value": "KEY_F13" },
        "B": { "type": "SendCharAction", "name": "Main Scene",   "value": "KEY_F14" },
        "C": { "type": "SendCharAction", "name": "Mute Mic",     "value": "KEY_F15" },
        "D": { "type": "SendCharAction", "name": "Start Stream", "value": "KEY_F16" }
      }
    }
  ]
}
```

A multi-profile set — say, three DAW modes — uses the same format with additional entries in
the `"profiles"` array. The `index.json` manifest entry for a multi-profile set lists each
profile's name in a `"profiles"` field so the gallery can show what's inside before the user
loads it.

The `_meta` block is ignored by the firmware (it is a freeform `_doc`-style comment block).
CI reads it to generate `index.json`. This keeps all profile metadata co-located with the file
rather than split across a separate manifest that can go out of sync.

---

## CI / Validation

A lightweight CI step runs on any PR that adds or modifies files under `profiles/`:

1. **Schema validation** — validate each `.json` against `data/profiles.schema.json` using
   the existing `ajv` toolchain (already used by the config builder).
2. **`minButtons` consistency check** — verify that the number of buttons defined in the
   profile's `buttons` object does not exceed the subfolder's declared minimum.
3. **`index.json` regeneration** — read `_meta` blocks from all profile files (including the
   optional `minFirmware` field), rebuild `index.json`, and commit the result (or fail CI if
   the committed index is stale).
4. **No firmware changes required** — profile contributions never touch C++ code.

---

## Starter Profiles to Ship at Launch

These seed the gallery on day one and come from the existing `example.json`:

| File | Category | Min buttons | Status |
|---|---|---|---|
| `score-navigator.json` | Sheet music | 2 | Extract from `example.json` |
| `pixel-camera-remote.json` | Photography | 2 | Extract from `example.json` |
| `vlc-practice.json` | Video / practice | 2 | Extract from `example.json` |
| `obs-stream-deck.json` | Streaming | 4 | Extract from `example.json` |
| `ableton-looper.json` | DAW | 4 | Extract from `example.json` |
| `discord-comms.json` | Communication | 4 | Extract from `example.json` |
| `logic-pro-tracking.json` | DAW | 2 | Write new |
| `shutter-delayed.json` | Photography | 1 | Write new |
| `play-pause-media.json` | Universal | 1 | Write new |
| `zoom-mute-toggle.json` | Video calls | 2 | Write new |
| `podcast-control.json` | Podcasting | 3 | Write new |
| `lightroom-culling.json` | Photography | 4 | Write new |

---

## Call for Contributions

The `profiles/CONTRIBUTING.md` should include:

### What makes a good community profile

- **It solves a real workflow you actually use.** Not a hypothetical — something you pressed a
  button to do last week.
- **The button labels are descriptive.** "Record" is better than "Button A". "Mute Mic (Ctrl+Shift+M)"
  is better than "Mute".
- **The `description` field explains the app/context.** Future users should not have to guess
  what software this is for or where to configure the hotkeys it relies on.
- **`tested` is honest.** Only list the app and OS you have actually run it against.

### How to contribute

```
1. Fork the repository.
2. Copy profiles/4-button/obs-stream-deck.json (or whichever template matches
   your button count) to the correct subfolder.
3. Edit the JSON — update _meta.id, _meta.author, _meta.tags, the profile name,
   description, and button actions.
4. Validate locally: npm run validate-profiles (or use the online schema validator).
5. Open a Pull Request. CI will validate the schema and regenerate index.json.
```

### The social dimension

Every merged profile is credited to its author in `index.json` and displayed in the community
gallery inside the tools and the mobile app. Contributors appear alongside the profile every
time someone loads it. For a small open-source project, this is a concrete, permanent, visible
contribution that requires no C++ knowledge — just a use case and ten minutes.

---

## Open Questions

| # | Question | Impact |
|---|---|---|
| Q1 | Should each file contain one profile or allow multiple profiles (a full 7-profile set)? | **Resolved:** every file is a profile set (1–7 profiles). Single-profile sets are the common case; multi-profile sets are allowed for contributors who want to share a complete workflow. See the Terminology section. |
| Q2 | Should the `profiles/` folder be served via GitHub Pages (same origin as the tools) or raw.githubusercontent.com? | GitHub Pages is cleaner for CORS and caching; raw content works without extra setup. |
| Q3 | Should the mobile app cache profiles for offline use, or fetch on demand? | Offline-first is better UX but requires cache invalidation logic. |
| Q4 | Should profiles specify which firmware version they require? | **Resolved:** add `minFirmware` as an optional field to `_meta` from day one. The CI script and `index.json` generator include it when present; tools and the app surface it as a badge if set. Starter profiles omit it (implying "any firmware"). This costs nothing to add now and avoids a breaking schema change later. |

---

## Relationship to Other Ideas

- **IDEA-015 / IDEA-016 (Content Strategy)**: The profiles gallery is a concrete, scannable
  proof-point for every marketing claim. "Seven remappable profiles" becomes real when a visitor
  can click "Browse 20 community profiles" and immediately see OBS, Ableton, forScore, and
  Lightroom examples. Link the gallery prominently in the README and in the Maker-Musician article.
- **IDEA-006 (Macros)**: When MacroAction is production-ready, multi-step macro profiles become
  a compelling contribution category. The schema already supports `MacroAction` — the gallery
  just needs profiles that use it.
- **IDEA-010 (Double Press)**: Same — when double-press lands, the gallery structure already
  supports it via `doublePress` in the schema.
- **IDEA-001 (Mobile App)**: The community profiles browser is a natural screen in the app —
  it requires no BLE connection and can work as a discovery/onboarding step before pairing.
