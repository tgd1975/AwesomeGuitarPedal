---
id: IDEA-042
title: One-click profile contribution from the app to the community profiles repository
category: apps
description: A near-frictionless "Share this profile" flow inside the mobile app and web tools that submits a user's profile to the community repo for maintainer review — no Git, no fork, no PR knowledge required.
---

## The Idea in One Sentence

Inside the mobile app (and the web configurators), a user who has built a profile they like
taps **"Share with the community"**, fills in a tiny form (name, description, what app it works
with), and hits submit — the contribution then appears in a maintainer review queue, and on
approval lands in [`profiles/`](../open/idea-017-community-profiles-repository.md) and the
auto-generated `index.json`.

No fork. No clone. No `git`. No JSON editing. No knowledge that GitHub even exists, beyond
optionally entering a name to be credited as.

---

## Why This Matters

[IDEA-017](idea-017-community-profiles-repository.md) describes the destination — a curated
folder of community profiles served to all the tools and the app. But the contribution path
it currently sketches is:

> 1. Fork the repository.
> 2. Copy a template file into the right subfolder.
> 3. Edit the JSON — `_meta.id`, `_meta.author`, tags, button actions.
> 4. Validate locally: `npm run validate-profiles`.
> 5. Open a Pull Request.

That is a five-step Git-literate flow. The realistic target audience — musicians, streamers,
photographers, podcasters — will not do this. Even developers won't bother for a single profile.
The repo will fill up with profiles authored by the maintainer and approximately nobody else,
and the "community" framing will quietly become aspirational.

A one-tap **"Share this profile"** button inside the tool the user already has open removes
every step except the actual content. The user has *just built and tested* the profile they
want to share; the moment to ask is now, not after they have closed the app and learned Git.

The cost of being wrong here is low (a few weekends of plumbing). The cost of being right is
the difference between a real community gallery and an empty one.

---

## The User Flow (from the app)

1. User has just edited or pulled down a profile in the app and is happy with it.
2. From the profile detail / editor screen, they tap **"Share with the community"**.
3. A small form opens, pre-filled from the profile where possible:
   - **Name** (pre-filled from the profile name) — required.
   - **Short description** — required, ~140 chars, with a placeholder showing a good example.
   - **What app / workflow is this for?** — required, free text (e.g. "OBS Studio 30 on macOS").
   - **Tags** — optional, suggested from a small predefined set (`streaming`, `daw`,
     `sheet-music`, `photography`, `podcasting`, `video-calls`, `accessibility`, ...).
   - **Tested on** — optional, free text (e.g. "Logic Pro 11, macOS 15").
   - **Credit me as** — optional, free text. Default: blank → "Anonymous".
   - **Email (optional)** — only shown if the user wants to be notified when it is reviewed.
4. The user taps **Submit**. The app shows "Thanks — your profile is queued for review" and
   returns them to the editor.
5. Behind the scenes, the profile JSON plus the form metadata is sent to a tiny submission
   endpoint (see "Backend options" below).
6. A maintainer reviews it, accepts (with optional edits) or rejects (with a reason). On
   accept, it lands in `profiles/<min-buttons>/<slug>.json` and `index.json` regenerates.
7. If the user provided an email, they get a one-line "your profile is now live" or "we passed
   on this one because X" message.

The contributor never sees Git, JSON, or a Pull Request.

---

## What the Maintainer Sees

A simple review queue — not a custom web app, ideally. Two viable shapes:

**Option A — GitHub Issues as the queue.** The submission endpoint opens an Issue in the repo
with a structured body (the profile JSON in a code block, plus the form fields rendered as a
checklist). The maintainer reviews on GitHub, edits if needed, and runs a one-shot script
(or a "merge this submission" GitHub Action triggered by an issue label) that:

- writes the JSON to `profiles/<n>-button/<slug>.json`,
- regenerates `index.json` (the existing IDEA-017 CI step),
- closes the issue with a "merged as `<commit-sha>`" comment.

**Option B — Pending PRs as the queue.** The endpoint opens a PR directly (via a bot account)
with the new profile file already in the right path. The maintainer reviews the diff, edits in
GitHub's web UI if needed, merges. Closer to "normal" Git, slightly more setup, but the review
experience is exactly the standard PR flow with diff highlighting.

Both options keep maintainer effort low (review + one click) and keep `main` clean — the user's
contribution is provisional until it is explicitly accepted.

---

## Backend Options (rough — to be decided)

The app needs to send the submission *somewhere*. Three plausible shapes, in increasing order
of infrastructure:

1. **GitHub REST API directly from the app, with a fine-scoped token.**
   The simplest. The app calls `POST /repos/.../issues` (Option A) or creates a branch +
   commits + PR (Option B) using a token with very narrow scope (issue creation only, in
   Option A). Risk: the token has to live in the app binary. Mitigation: use a token that
   *only* allows opening issues in *one* repo, in a hidden sub-area, and rate-limit on the
   GitHub side. For Option A this is acceptable — the worst-case abuse is spam issues, which
   the maintainer can label and a workflow can auto-close.

2. **A tiny serverless proxy** (Cloudflare Worker / Vercel function / GitHub Action with
   `repository_dispatch`). The app POSTs to the proxy; the proxy holds the GitHub token
   server-side and creates the issue/PR. Adds one moving part but removes the embedded-token
   problem.

3. **An email-based submission** (mailto: or a forms service like Formspree). Lowest tech
   bar, but the maintainer has to manually convert each email into a profile file. Ruled out
   unless the volume is so low that automation is overkill.

Recommendation for a first version: **Option A (Issues queue) with GitHub REST API directly
from the app**, using a narrowly-scoped token. If abuse becomes a problem, swap in the
serverless proxy without changing the user-facing flow.

---

## What Gets Submitted

Two pieces:

1. **The profile JSON itself**, exactly as defined by `data/profiles.schema.json` — the same
   format the firmware loads. The app already has this in memory; it is what is currently
   being edited.
2. **The form metadata** — name, description, target app, tags, tested-on, credit-as,
   optional email. The maintainer merges this into the profile's `_meta` block during review,
   producing the final on-disk format described in IDEA-017.

The app should *not* try to write a final `_meta` block itself — keeping that step on the
maintainer's side avoids the need to ship complex slug/metadata logic in the client and means
the maintainer always has a chance to clean up tags and naming before the profile becomes
public.

---

## Validation in the App, Before Submission

Cheap pre-checks that prevent obviously broken submissions from ever reaching the queue:

- The profile validates against `data/profiles.schema.json` (the app already does this as
  part of normal editing).
- The required form fields are filled in.
- The description is at least, say, 20 characters and is not "asdf".
- The profile contains at least one configured button.
- A soft duplicate check: if the profile name matches an existing community profile, warn the
  user and ask whether they want to submit anyway.

None of these are security checks — they are friction-savers for the maintainer.

---

## Privacy and Attribution

- Default attribution is **Anonymous**. Users opt in to being credited.
- The "Credit me as" field is *not* tied to a GitHub account. A user can write "Sarah from
  Berlin" or "tgd1975" or leave it blank. The community gallery shows whatever string they
  chose; if it happens to match a GitHub username, the gallery may optionally link to it, but
  this is purely cosmetic.
- Email, if provided, is used once for the accepted/rejected notification and then discarded.
  It is not stored in the public repo.
- The submission flow makes both of the above explicit on the form ("Your email is used only
  to notify you about this submission and is not shared.").

---

## Anti-Abuse, Lightly

The threat model is "people submitting jokes, slurs, or copy-paste spam", not nation-state
adversaries. Cheap mitigations:

- Rate-limit submissions per device (e.g. 5/day) in the app.
- The maintainer is the gate — nothing reaches the public repo without manual review.
- Submissions that are obviously spam can be auto-closed by a label-based workflow.

If/when volume grows, add a hCaptcha on the serverless proxy (Option 2 above).

---

## Web Tools Parity

The same flow should exist in the web Config Builder and the Profile Builder / Simulator.
Browser users have the same friction problem — "fork the repo and open a PR" is no easier from
a browser than from a phone. The submission UI is the same form; the backend path is the same
(GitHub Issue or PR via the proxy / direct API call).

---

## Open Questions

| # | Question | Impact |
|---|---|---|
| Q1 | Issues queue (Option A) or PR queue (Option B) for the maintainer? | A is simpler; B gives diff-based review out of the box. |
| Q2 | Direct-from-app GitHub API call or serverless proxy? | Direct = zero infra; proxy = no embedded token. |
| Q3 | Should the app retain a "my submissions" list so users can see status without an email? | Nice-to-have; needs a way to look up issues by submitter without an account. |
| Q4 | What is the fallback if the submission endpoint is unreachable? Save locally and retry, or fail loudly? | Saving + retry is friendlier; failing loudly is honest. |
| Q5 | Should accepted submissions also push back into the user's app (so their profile shows the assigned `_meta.id` and "this one is in the community gallery now")? | Closes the loop nicely; needs the app to know its own submissions. Could be deferred. |
| Q6 | Where does the GitHub token live for Option A direct calls — bundled in the binary, fetched on first run, or per-user OAuth? | Bundled = simplest, lowest security; OAuth = clean but heavyweight UX. |

---

## Relationship to Other Ideas

- **IDEA-017 (Community Profiles Repository)** — this idea is the *contribution side* of
  IDEA-017. IDEA-017 designs the destination and the gallery; this idea designs the on-ramp.
  The two should ship close together, because IDEA-017 without this is mostly a maintainer-
  authored folder.
- **IDEA-015 / IDEA-016 (Marketing / Articles)** — every successful one-click submission is a
  small word-of-mouth event. "I shared my profile" is itself shareable in a way that "I forked
  a repo and opened a PR" is not.
- **IDEA-001 (Mobile App)** — the share button is a natural addition to the profile editor
  screen. It does not require BLE and works against the user's local in-memory profile.
- **IDEA-037 (App content pages)** — the in-app "How-To" / About pages can include a short
  "Share your profile" explainer that links to the gallery, closing the loop visually.

---

## Out of Scope (for the first version)

- Voting, likes, or comments on community profiles. Curated > social.
- User accounts inside the app. Anonymous-by-default is the point.
- Automated merges without maintainer review. The review step is what makes the gallery
  trustworthy; removing it is a different idea.
- Editing or deleting an already-merged profile from inside the app. Once it is in the repo,
  changes go through normal Git — but realistically, this will affect ~1% of submissions and
  can be handled on request.
