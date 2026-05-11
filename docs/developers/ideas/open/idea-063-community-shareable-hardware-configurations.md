---
id: IDEA-063
title: Community-shareable hardware configurations
description: Mirror the community-profiles flow for hardware configs so builders can share known-good config.json files tagged by parts list / build.
category: 📱 apps
---

## Motivation

We already have a community-profiles flow (service in
[`app/lib/services/community_profiles_service.dart`](../../../../app/lib/services/community_profiles_service.dart),
screen in
[`app/lib/screens/community_profiles_screen.dart`](../../../../app/lib/screens/community_profiles_screen.dart),
contribution guide in
[`profiles/CONTRIBUTING.md`](../../../../profiles/CONTRIBUTING.md)),
but **hardware configs are not shareable**. Two builders with the same
parts list have to re-derive the same `config.json`
(`data/config.json`, schema in `data/config.schema.json`, model in
[`app/lib/models/hardware_config.dart`](../../../../app/lib/models/hardware_config.dart))
independently. Profiles get the community treatment; hardware configs
do not.

**Prerequisite:** IDEA-060 (debounce time in hardware config). The
shared `config.json` needs to be a complete description of a build,
including the debounce value of the physical switches — without that,
shared configs are incomplete.

## Rough idea

Mirror the community-profiles flow for hardware configs. The two are
**user-facing distinct** — hardware configs describe the *physical
build* (pins, switch debounce, BLE pairing pin), profiles describe
*sound presets*. Users must be able to tell them apart at a glance, so
they live behind separate entry points in the app and have their own
contribution flow.

**Implementation-wise, KISS** — reuse the existing community-profiles
plumbing (docs-site publishing pipeline, fetch service shape, index
format) rather than inventing a parallel stack. Concretely:

- A `hardware-configs/` directory in the same docs-site publishing
  pipeline as `profiles/`.
- An index file with the same shape as the profiles index, fetched by
  a service modelled on `community_profiles_service.dart`.
- An app screen analogous to `community_profiles_screen.dart` for
  browsing and importing — separate screen, same patterns.
- A `CONTRIBUTING.md` analogous to
  [`profiles/CONTRIBUTING.md`](../../../../profiles/CONTRIBUTING.md).

A builder publishes their working `config.json` tagged with the parts
list / build it matches. Other builders reproducing the same build
can drop it in.

Schema validation already exists. Profile ↔ hardware-config pairing is
explicitly out of scope; the two stay independent in the UI as today.
