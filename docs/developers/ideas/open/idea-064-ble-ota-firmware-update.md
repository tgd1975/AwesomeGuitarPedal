---
id: IDEA-064
title: BLE OTA firmware update — app pushes latest compatible firmware to pedal over Bluetooth
description: BLE-based over-the-air firmware update flow so users can keep the pedal on the latest compatible firmware without USB or Wi-Fi.
category: ⚡ firmware
---

## The idea

Today, updating the pedal firmware requires a USB cable and a host with PlatformIO (or a pre-built binary plus `esptool` / `adafruit-nrfutil`). For non-developer users that is a hard wall. The app already talks to the pedal over BLE — it should be able to deliver firmware updates over the same link.

**No Wi-Fi.** The pedal has no Wi-Fi configuration UI, no provisioning flow, and adding one would expand the attack surface, the BOM cost (on platforms where Wi-Fi is optional), and the user-onboarding burden. "Over the air" here means *over BLE*.

The user-visible flow we want:

1. App connects to the pedal as usual.
2. App reads the pedal's hardware platform (ESP32 / nRF52840), current firmware version, and any compatibility identifiers from the device.
3. App fetches the release manifest from the project's release channel (GitHub Releases is the obvious starting point — the same place [release.yml](../../../../.github/workflows/release.yml) publishes builds today).
4. App picks the *latest firmware build that is compatible* with the connected pedal (right platform, right hardware revision, satisfies any min-app-version constraints — see IDEA-054).
5. User confirms.
6. App streams the firmware image to the pedal over BLE, the pedal verifies and applies it, reboots into the new image.

## Open questions

- **Bootloader / DFU stack per platform.**
  - nRF52840: Nordic's MCUBoot or the legacy SoftDevice DFU service is the well-trodden path; Adafruit Bluefruit boards already ship with a serial+BLE bootloader. How much of that do we keep vs replace?
  - ESP32: there is no first-party BLE DFU equivalent. Options include rolling our own update service on top of the existing GATT stack, using something like `esp_https_ota` adapted for BLE transport, or a custom chunked-write characteristic with CRC. None are free.
- **Image signing.** Without signing, any BLE-pairable peer can push an arbitrary image and brick / hijack the pedal. Signing is non-optional, but introduces a key-management story (where does the public key live in firmware, how do we rotate it, what happens to in-the-wild pedals if the signing key leaks).
- **Compatibility matching.** What identifies a "fitting" firmware? Platform (esp32/nrf52840) is necessary but not sufficient — there are also hardware variants (4-button prototype, 2-button rugged, future variants), config-schema versions, and app↔firmware compatibility. IDEA-054 already exists for the cross-version app/firmware question; this idea depends on the policy from there.
- **Release-manifest format and host.** GitHub Releases assets are the obvious start, but we'd need a machine-readable manifest (versions, platforms, hardware revisions, sha256, signature, min-app-version) that the app reads. JSON file in the release? Separate `latest.json`? Reuse an existing convention?
- **Failure modes.**
  - Transfer is slow over BLE — a multi-hundred-KB image will take minutes. The user must be able to leave the screen / lock the phone without bricking the pedal. Resumability is desirable.
  - If the new image fails to boot, the pedal must roll back. MCUBoot does this for us on nRF52840; on ESP32 we'd need dual-partition + boot-verify wired up (`esp_ota_*` API).
  - What if the user disconnects mid-flash? The pedal must be left in a known-good state — either still running the old image, or in a recoverable bootloader mode.
- **App UX.** When does the app *offer* the update vs auto-apply? Default to "notify, don't auto-update"? How prominent is the indicator?

## Rough approach (not a plan)

A first cut might be:

1. Decide platform-by-platform: lean on MCUBoot/Bluefruit bootloader for nRF52840 first (lower lift), and treat ESP32 as a second phase once the nRF flow is proven.
2. Define the release-manifest schema and have [release.yml](../../../../.github/workflows/release.yml) emit it alongside the existing assets.
3. Add a BLE GATT service in the firmware that exposes firmware version, hardware identity, and the update transport.
4. Add an "Update firmware" flow to the app that reads the manifest, matches against the connected pedal, and runs the transfer.
5. Image signing from day one — even if key management is rough, unsigned OTA is not shippable.

## Related

- [IDEA-054](idea-054-cross-version-app-firmware-compatibility-policy.md) — cross-version app/firmware compatibility. OTA needs the answer from there to know what "fitting" means.
- [release.yml](../../../../.github/workflows/release.yml) — current release pipeline; the manifest emission would extend this.
- [BUILD_GUIDE.md](../../../musicians/BUILD_GUIDE.md) and developer-side flashing docs — the USB-cable path remains the supported escape hatch for developers and recovery.
