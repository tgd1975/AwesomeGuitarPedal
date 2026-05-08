---
id: TASK-343
title: Add GitHub social preview image
status: closed
closed: 2026-05-01
opened: 2026-04-30
effort: Small (<2h)
effort_actual: Small (<2h)
complexity: Junior
human-in-loop: Clarification
epic: logo-branding
order: 5
---

## Description

When the repo is shared on Twitter/Discord/Slack, GitHub serves a generic
preview card because no social preview is configured. Add an `og:image` /
`twitter:image` to the gh-pages HTML surfaces (simulator and config builders),
and upload a 1280×640 social preview to the GitHub repo settings so the
repository's own share card uses the ASP mark.

Per IDEA-049 this is the "sanity check" item — small polish with disproportionate
effect on perceived cohesiveness.

## Acceptance Criteria

- [ ] A 1280×640 social preview PNG generated from `asp-mark.svg` (mark + wordmark on background) and committed under `docs/design/handoff/logo/`
- [ ] `og:image` and `twitter:image` meta tags added to simulator and both config builder HTML entry pages, pointing at the committed PNG via an absolute URL (gh-pages)
- [ ] User uploads the same PNG to the repo's GitHub *Settings → Social preview* (human-in-loop step — agent cannot do this)

## Test Plan

No automated tests required — change is non-functional (meta tags + asset).

Manual verification: paste the gh-pages URL into Twitter's Card Validator or
the Open Graph debugger and confirm the preview image renders. After the user
uploads to GitHub Settings, paste the repo URL into the same validator.

## Notes

- The GitHub Settings upload step requires repo admin access via the web UI; the agent should pause on the final AC and ask the user to do that step.
- **Design conformance is the law.** Compose the 1280×640 from the canonical assets in [`docs/design/handoff/logo/`](../../../../docs/design/handoff/logo/) — most likely [`asp-lockup.svg`](../../../../docs/design/handoff/logo/asp-lockup.svg) (mark + wordmark horizontal) on the system background colour. Do not redraw, recolour, or simplify the mark or wordmark (handoff README non-negotiables #5 and #6). Background and spacing should follow [`docs/design/ASP-design-review.html`](../../../../docs/design/ASP-design-review.html); colours come from [`docs/design/handoff/tokens.css`](../../../../docs/design/handoff/tokens.css). See EPIC-024 "Design authority" for the full rule set.

## Paused

- 2026-05-01: Image generated and meta tags wired up in commit a7cb683.
  Final AC (`/settings → Social preview` upload) requires repo admin
  web-UI access — the agent cannot perform it. Resume with
  `/ts-task-active TASK-343` after the upload, then close.
