---
id: IDEA-055
title: Senior Designer agent — audit app, web tools, and docs against the design style guide
description: A reviewer-persona agent (analogous to the senior architect / senior journalist) that checks visual artifacts for conformance to docs/design/.
category: 🛠️ tooling
---

## Motivation

The project already has reviewer-persona agents that bring outside-expert
judgement to specific artifacts:

- **Senior software architect** — reviews code/architecture against
  project conventions.
- **Senior journalist** — reviews persona-facing prose for tone, clarity,
  and audience fit.

The visual layer has no equivalent. The design style guide lives in
[`docs/design/`](../../../design/) (`ASP-design-review.html` / `.pdf`,
`README.md`, `handoff/`), but nothing systematically checks whether the
Flutter app, the web tooling (simulator, builders), generated PDFs, and
other docs actually follow it. Drift accumulates silently — fonts that
crept in, off-palette colours, inconsistent spacing, ad-hoc icon sets,
tone mismatches between marketing copy and the guide.

A **Senior Designer** agent closes that gap. Same shape as the existing
reviewer agents: focused expertise, invoked on demand, returns a structured
critique rather than making changes.

## Rough scope

What the agent reviews:

- **Flutter app** — screens, components, theme tokens (colours, typography,
  spacing scale), iconography, empty states, error states.
- **Web tooling** — simulator UI, the two builders (basic / advanced),
  shared header/footer, version badge, any landing pages.
- **Generated docs** — PDFs produced from Markdown, BUILD_GUIDE,
  persona-facing pages once they go visual.
- **Marketing / outreach assets** if and when they exist (IDEA-015 lineage).

What it checks against:

- `docs/design/ASP-design-review.html` and `.pdf` — the canonical visual
  reference.
- `docs/design/README.md` — narrative rationale and rules.
- `docs/design/handoff/` — tokens, component specs, any export bundles.

Dimensions in the report:

- **Typography** — type scale, font choice, weight/size pairings, line-height.
- **Colour** — palette adherence, contrast, semantic use (success/warn/error).
- **Spacing & layout** — grid, rhythm, density.
- **Components** — buttons, inputs, cards, modals — match the documented
  spec.
- **Iconography** — single set, consistent style and weight.
- **Tone in UI copy** — labels, empty states, error messages.
- **Cross-surface consistency** — same component renders the same in app
  and web tools.

## Open questions

- **Input format.** Does the agent need rendered screenshots (Flutter golden
  tests, headless browser captures of the web tools, PDF page renders), or
  can it work from source (theme files, CSS, Markdown) plus the style guide?
  Probably both: source for rules, screenshots for actual rendering.
- **Invocation.** Slash command (`/design-review`)? Subagent type used
  inside other reviews? Both? The journalist and architect models give two
  precedents to choose from.
- **Output shape.** A markdown report with sections per dimension and
  per-finding severity? A JSON-ish structured list other tooling can
  consume? Mirror whichever shape the existing reviewer agents settled on.
- **Scope per run.** Whole project (slow, thorough) vs. "just this PR's
  visual diff" (fast, targeted). Likely both modes, like the existing
  review skills.
- **Relationship to IDEA-038** (comprehensive review framework). Design is
  one of the dimensions that framework will eventually cover — this idea
  could either be a standalone agent that IDEA-038 then incorporates, or
  scoped from the start as the "design dimension" of that framework.
- **Style guide gaps.** The first few runs will surface things the guide
  doesn't actually specify. The agent should flag those as "guide gap"
  rather than "violation", so they feed back into improving
  `docs/design/`.

## Why now (or later)

Not urgent — the visual surface is small enough today that drift is still
catchable by eye. But it grows: EPIC-023 content pages, the simulator
polish, eventual marketing pages, persona-facing PDFs. Once there are
five or six surfaces, manual review stops scaling and the guide stops
being enforced.
