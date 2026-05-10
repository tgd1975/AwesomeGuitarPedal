---
id: IDEA-045
title: Validation banner revalidates when an action is edited (not only when profile count changes)
category: apps
description: The Profile List validation banner currently only re-runs validation when the number of profiles changes. Editing a button's action value (e.g. fixing a typo) does not refresh the banner — it stays red until the user navigates away and back. Trigger revalidation on any ProfilesState change.
---

## Archive Reason

2026-04-28 — Converted to TASK-281.

## The Idea in One Sentence

Drop the `count != _lastProfileCount` cache check in
`ValidationBanner` and revalidate on every `ProfilesState` notification
(or at least on every action-mutation notification), so users see the
banner clear immediately after fixing a bad value instead of having to
exit and re-enter the Profile List.

---

## Why This Matters

Surfaced while verifying TASK-264 + TASK-265 on 2026-04-27. After
fixing `value: "NOT_A_VALID_KEY_VALUE"` → `value: "KEY_PAGE_UP"` in
the Action Editor and tapping Save, the Profile List banner kept
reading "1 error" until the user backed out to Home and re-entered
Profile List, at which point it correctly turned green.

The current trigger logic in
[app/lib/widgets/validation_banner.dart](../../../app/lib/widgets/validation_banner.dart)
revalidates only when the **count** of profiles changes:

```dart
if (count != _lastProfileCount) {
  _lastProfileCount = count;
  WidgetsBinding.instance.addPostFrameCallback((_) => _validate(state));
}
```

This was fine when validation was schema-only (a missing field can
only appear / disappear when a profile is added / removed). With the
TASK-264 runtime-resolvability check, an action's `value` can flip
between resolvable and unresolvable without changing the profile
count — and the banner becomes stale.

---

## Possible Approach

Watch the whole `ProfilesState` for changes and revalidate on each.
The cheapest change:

```dart
@override
void didChangeDependencies() {
  super.didChangeDependencies();
  final state = context.read<ProfilesState>();
  state.addListener(() => _validate(state));
}
```

(remember to remove the listener in `dispose`).

If validation is expensive on large profile sets, debounce by
~250 ms instead of running on every keystroke during an action edit.

## Out of Scope

- Changing the banner's visual design.
- The cross-process question of whether validation should also re-run
  after an upload (it currently doesn't — but that's a different
  surface).

---

## References

- TASK-264 (validator coverage of unresolvable values) — closed.
- TASK-265 (Action Editor crash on the same input) — closed.
- Surfaced 2026-04-27 during the on-device verification round of
  TASK-264 + TASK-265.
