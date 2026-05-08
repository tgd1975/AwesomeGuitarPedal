---
id: IDEA-052
title: Localize the Flutter app — scaffold flutter_intl and translate content pages
description: Revisit localization once the EPIC-023 content pages stabilise; scaffold flutter_intl and translate at minimum the How-To and Troubleshooting pages.
category: 📱 apps
---

EPIC-023 ships English-only to keep the initial content-page work small.
This idea captures the deferred localization work so it isn't lost.

## Motivation

Musicians outside English-speaking markets are a real share of the
target audience (DIY-pedal communities are large in DE/FR/JP/ES). The
two pages that benefit most from translation are:

- **How-To** — first-time pairing instructions, where unfamiliar
  English error states already cause friction.
- **Troubleshooting** — by definition read while frustrated, where
  comprehension matters most.

The Info/About, Profiles explainer, and Legal pages are lower priority
(short, technical, link-heavy).

## Rough approach

1. Scaffold `flutter_intl` (or `flutter_localizations` + ARB files
   directly) once the content-page copy has stabilised — i.e. after
   TASK-331..TASK-335 have landed and the wording is no longer in flux.
2. Extract existing English strings into ARB.
3. Pick an initial second language (likely German, given the project's
   current contributor base) and machine-translate as a baseline,
   then have a native speaker review.
4. Add a language picker (auto-detect from device locale, override in
   settings).

## Open questions

- Translate copy ourselves vs accept community PRs (Crowdin / Weblate)?
- How to keep translations in sync as content drifts during development?
- Do we localize developer-facing pages (Legal, credits) or only
  user-facing ones?

## Why deferred

Scaffolding `flutter_intl` from day one would slow the initial
content-page work without delivering value while there is exactly one
language. Re-extracting strings later is a mechanical refactor,
cheaper than carrying ARB overhead through every copy iteration.

Decided in TASK-330 (EPIC-023 decision doc).
