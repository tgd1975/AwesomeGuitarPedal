---
id: TASK-160
title: Publish app to Google Play Store
status: open
opened: 2026-04-19
effort: Large (8-24h)
complexity: Medium
human-in-loop: Main
epic: distribution
order: 2
---

## Description

Prepare and publish the Flutter app to the Google Play Store as an open-source / hobby
release. This covers account setup, store listing, signing, release build, and the initial
submission.

## Pre-conditions

- Google Play Developer account ($25 one-time fee)
- App passes all Android feature tests (TASK-153–TASK-157)
- App icon, screenshots, and short/long description ready (ideally after TASK-162 design work)
- Privacy policy URL (required by Google Play for apps that request Bluetooth permission)

## Steps

### 1 — Account & app registration

- [ ] Create Google Play Developer account at play.google.com/console
- [ ] Create a new app entry; set package name `com.awesomestudiopedal.app`
  (check `app/android/app/build.gradle.kts` for current `applicationId` and align)
- [ ] Complete the app content rating questionnaire
- [ ] Add a privacy policy URL (can be a simple GitHub Pages page or README section)

### 2 — Signing

- [ ] Generate a release keystore:

  ```bash
  keytool -genkey -v -keystore upload-keystore.jks \
    -keyalg RSA -keysize 2048 -validity 10000 \
    -alias upload
  ```

- [ ] Store `upload-keystore.jks` **outside the repo** (never commit)
- [ ] Create `app/android/key.properties` (gitignored):

  ```
  storePassword=<password>
  keyPassword=<password>
  keyAlias=upload
  storeFile=<absolute path to upload-keystore.jks>
  ```

- [ ] Wire signing config into `app/android/app/build.gradle.kts`
- [ ] Verify `make flutter-build` (or `flutter build apk --release`) uses the release key

### 3 — Store listing

- [ ] App title: "AwesomeStudioPedal"
- [ ] Short description (80 chars max): "BLE foot pedal controller for musicians — configure button actions from your phone"
- [ ] Full description (4000 chars max): see docs/musicians/ for inspiration
- [ ] Category: Music
- [ ] Upload at least 2 phone screenshots (from TASK-157 feature tests or dedicated screen captures)
- [ ] Upload feature graphic (1024×500 px)
- [ ] Upload app icon (512×512 px, PNG, no rounded corners — Play Store adds them)

### 4 — Release build & upload

- [ ] `flutter build appbundle --release` (`.aab` is required for Play Store; `.apk` is sideload only)
- [ ] Upload `.aab` to the Internal Testing track first
- [ ] Install via Play Store internal testing link; run smoke test
- [ ] Promote to Open Testing or Production once smoke test passes

### 5 — Permissions declaration

Google Play requires a declaration for `BLUETOOTH_SCAN` / `BLUETOOTH_CONNECT` (Android 12+):

- [ ] In Play Console → App content → Permissions: declare BLE is used to communicate with
  the hardware pedal device; no location data is derived

## Acceptance Criteria

- [ ] App available on Internal Testing track in Google Play Console
- [ ] App installable on a fresh device via Play Store link
- [ ] Store listing complete with description, screenshots, and icon
- [ ] Signing key stored securely outside the repo; documented in a private note or password manager
- [ ] Privacy policy linked from the Play Store listing

## Notes

- Use App Bundle (`.aab`) not APK for Play Store submission
- The `applicationId` in `build.gradle.kts` must be unique and final — changing it after
  publishing creates a new app
- Keep the keystore and `key.properties` out of version control — add to `.gitignore`

## Design notes (extracted from TASK-179 planning, 2026-05-02)

These notes capture decisions and context from the TASK-179 design discussion (adding
the Android APK to the GitHub release pipeline). They're recorded here because they
directly inform Steps 2 and 4 of this task.

### Distribution channel split

Both APK and AAB get built per release, serving different channels:

```
Source ──┬── flutter build apk       ──→ GitHub Release  (sideload — debug-signed APK shipped via TASK-179)
         └── flutter build appbundle ──→ Play Console    (Play Store — this task)
```

Same source, same version (lockstep via `app/pubspec.yaml`). Different signing,
different distribution. APK and AAB are not "old vs new format" — they're parallel
outputs for parallel channels and both continue to exist long-term.

### Sideload ↔ Play Store migration — not pursued

Decision: **don't engineer in-place migration** between the GitHub-sideload APK and
the Play Store AAB. Reason: AwesomeStudioPedal has no Android user base to migrate.
Sideload and Play Store are treated as independent distributions; users pick one
channel and don't move between them.

Practical consequence: the keystore set up in Step 2 of this task is *only* the Play
upload key — it is not shared with TASK-179's sideload APK (which stays
debug-signed). Different signing → Android treats them as different apps → no
in-place upgrade between channels. Acceptable given zero existing users.

If a sideload user base ever materializes before Play onboarding, revisit before
submitting — the "Use the existing key" option in Play App Signing onboarding can
preserve in-place upgrades from sideload to Play, but it requires the sideload
signing key to be the same as the Play upload key from day one.

### Signing model — context for Step 2

Play App Signing has been the default for new apps since August 2021. Two key types
in play:

- **Upload key** — what you (or CI) sign the AAB with before uploading
- **App signing key** — what end-user devices verify against; Google generates and
  holds this when you onboard to Play App Signing

The keystore generated in Step 2 becomes your **upload key**. You never touch the
app signing key. Recovery: if the upload key is lost, Google can help re-register a
new one. (For comparison, in legacy self-managed signing, losing the single signing
key permanently bricks the app — no updates possible.)

**Use Play App Signing in Play Console onboarding.** Don't opt for "self-manage all
keys" — it's a strictly worse failure mode for negligible benefit.

### Upload to Play Console — three options

| | A. Local manual | B. CI builds AAB, manual upload | C. Full CI auto |
|---|---|---|---|
| **AAB build** | Local `flutter build appbundle` | release.yml workflow artifact | release.yml workflow artifact |
| **Upload step** | Browser → Play Console UI | Download from Actions tab → Play Console UI | `r0adkll/upload-google-play` action via Google service account JSON |
| **Secrets needed** | None | None | Google service account JSON (more sensitive than keystore — can publish on your behalf) |
| **Effort** | Lowest | Low | Medium (service account + Play API permissions) |

**Recommendation: option B for the first ~3 releases, then upgrade to C.**

Reason: first Play submission has many human-in-loop steps that have nothing to do
with the AAB upload itself — store listing, screenshots, content rating
questionnaire, privacy policy URL, data safety form, target audience declaration,
testing track configuration. Automating just the upload step buys little value
relative to the manual work surrounding it. Wait until the manual flow is muscle
memory before automating.

### AAB never lives on the GitHub Release page

For option B, the AAB is uploaded by release.yml as a **GitHub Actions workflow
artifact** (90-day retention, login required to download), NOT attached to the
public GitHub Release. Reason: a non-technical user browsing the Release page would
see two Android files (`.apk` + `.aab`), pick the wrong one, and get "no app can
open this file" when they tap the AAB on their phone. End users only ever see the
APK; the AAB is internal-only until it lands in Play Console.

### applicationId — flexible timing

Current value: `com.example.awesome_studio_pedal` (Flutter scaffold default).
Play Console rejects `com.example.*`, so the rename to `com.awesomestudiopedal.app`
(already named in Step 1 of this task) must happen before first Play submission.

Decided to leave the rename **out of TASK-179** and keep it in this task — the
rename is coupled to Play readiness, not to "ship a debug-signed APK on GitHub."
Since there's no user base, timing is flexible: the rename can happen any time
between now and Step 1 of this task without breaking anyone.

### Open questions for this task

- Should the `flutter build appbundle` job in release.yml be added as part of this
  task, or as a separate prerequisite task that just adds the build pipeline
  (without the upload mechanics)? Defaulting to "as part of this task" since the
  AAB-build wiring is tightly coupled to the Step 2 signing setup.
- Per-release notes synchronization: GitHub Release body is auto-generated by
  release.yml from CHANGELOG; Play Console release notes are entered manually. If
  release cadence becomes high enough to feel repetitive, worth scripting.
