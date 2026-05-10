---
id: IDEA-059
title: Reduced and expert views for profile screens — app and web configurator
description: Default to a reduced view of profile screens that hides expert-only knobs; full view is opt-in. Config editor is unaffected — editing implies expert.
category: 📱 apps
---

## Motivation

The profile-facing surfaces (Flutter app profile screens, web configurator's
profile pages) currently expose every knob a profile carries. For a user who
just wants to pick a profile and play, that surface area is intimidating —
most fields are only relevant to power users tweaking edge behaviour.

Idea: introduce a **reduced view** as the default, and an **expert view** as
an opt-in toggle. The reduced view shows only the fields a typical user
needs to identify, choose, and use a profile. The expert view shows
everything.

## Scope — and what's deliberately out

In scope:

- Profile listing / detail screens in the Flutter app.
- Profile pages in the web configurator (the read/browse side).

Out of scope:

- The **config editor**. If you're editing a config, you're already an
  expert — degrading that surface with a "reduced" mode would make it
  worse, not better. No toggle there.

## Open questions

- **What counts as "reduced" vs "expert"?** Per-field classification needs
  doing. First cut: name + description + button-action summary in
  reduced; raw HID codes, validation flags, internal IDs, etc. in expert.
- **Where does the toggle live?** Per-screen toggle, global setting, or
  remembered-per-user? Global with a remembered preference is probably
  cheapest.
- **Two surfaces, one taxonomy?** The app and the web configurator should
  agree on which fields are "expert" so users don't get a different view
  on different surfaces. Suggests a shared classification (frontmatter on
  the profile schema, or a sibling enum) rather than two parallel lists.
- **Discoverability of expert mode.** A user who's outgrown reduced needs
  to find the toggle. Does it sit in settings? A small "show all" link
  on each profile page?
- **Interaction with [IDEA-053](idea-053-context-sensitive-helper-system.md).**
  The context-sensitive helper could surface "this field is hidden in
  reduced view — switch to expert to see it" guidance. Worth coordinating
  before either lands.

## Rough approach

1. Classify each profile field as `reduced` or `expert` (one canonical
   list, used by both surfaces).
2. App: gate expert fields behind a `showExpertView` flag, persisted in
   user settings.
3. Web configurator: same flag, same persistence story (localStorage or
   user account, depending on what's already there).
4. Config editor: leave alone.

## Why this matters

The audience for the project explicitly includes non-technical musicians
who want to load a profile and play. Today the profile surfaces are built
for the developer/builder persona. A reduced view lowers the floor without
raising the ceiling.
