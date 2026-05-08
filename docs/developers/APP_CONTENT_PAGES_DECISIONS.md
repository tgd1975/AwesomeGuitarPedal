# App Content Pages — Design Decisions

Foundation decisions for [EPIC-023](tasks/active/epic-023-app-content-pages.md) (App content pages and connection visibility), seeded by IDEA-037. These resolve the four open questions left by IDEA-037 so all subsequent content-page tasks (TASK-331..TASK-338) build on a shared foundation.

Decided in [TASK-330](tasks/active/task-330-decide-content-page-open-questions.md), 2026-05-01.

## 1. Source of truth for content — re-author per page in Dart strings

App copy is **app-specific** — written for phone screens, free of developer jargon, paced for someone mid-setup. Curated app copy is not the same artefact as the developer-facing or builder-facing markdown under `docs/users/` and `docs/developers/`.

**Rationale:** the cost of keeping two short, divergent copies in sync (app strings + docs prose) is lower than the cost of dragging a Markdown rendering layer into the app and forcing every doc page to be phone-readable. Phones and docs sites optimise for different reading patterns.

**How to apply:** content for Info/About, How-To, Profiles explainer, Troubleshooting, Legal lives in Dart files (or, later, ARB) — not loaded from a markdown asset bundle. Cross-reference docs from the app via tappable links where users may want to go deeper.

## 2. Localization — English only; revisit later

No `flutter_intl` scaffolding for the initial content-page ship. The app stays English-only.

**Rationale:** scaffolding ARB extraction now slows down every copy iteration during the period when content is most in flux. Re-extracting once strings stabilise is a mechanical refactor.

**How to apply:** plain Dart strings throughout. No `intl` imports. The deferred localization work is captured in [IDEA-052](ideas/open/idea-052-app-localization.md).

## 3. How-To context-awareness — static walkthrough

The How-To page is a static, top-to-bottom walkthrough. It does not change based on whether a pedal is currently paired, whether a send just failed, or any other live app state.

**Rationale:** KISS. A static page is straightforward to write, test, and translate later. A context-sensitive helper layer is genuinely useful but is its own design problem (state catalogue, banner widget, dismissal model) and shouldn't ride on the back of "ship a How-To page".

**How to apply:** TASK-332 (How-To page) implements a static walkthrough only. Inline state-aware affordances on other screens (e.g. "Tap to pair" prompts) are out of scope for EPIC-023 and tracked in [IDEA-053](ideas/open/idea-053-context-sensitive-helper-system.md).

## 4. First-run flow — auto-show How-To with one-tap dismiss

On first launch the How-To page is shown automatically with a prominent **"Got it, don't show again"** dismiss control. Subsequent launches go straight to the configurator. The How-To remains reachable via the menu at any time.

**Rationale:** target users are musicians installing a configurator app mid-setup; the 70% who would otherwise fumble for the menu benefit from the auto-show, and the 30% who hate intrusive onboarding can dismiss it once. This is the established pattern for utility apps with a non-trivial pairing step (Sonos, Hue, most BLE-pedal apps), so musicians won't be surprised by it. It also cuts support load.

**How to apply:** TASK-332 (How-To page) implements both the page and the first-run gate. The dismiss state is persisted via `shared_preferences` (or equivalent) under a single key (e.g. `howto_first_run_dismissed`). No checkbox-buried-in-settings — one tap and it's gone forever.
