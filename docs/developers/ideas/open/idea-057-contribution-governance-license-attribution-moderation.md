---
id: IDEA-057
title: Contribution governance — license, attribution, content moderation, and low-friction onboarding
description: Contribution governance — pick a license, attribute contributors, moderate content, and onboard non-technical users without forcing them to learn Git.
category: 📱 apps
---

Sibling to [IDEA-042](idea-042-one-click-profile-contribution-from-app.md) (one-click profile contribution from the app) and the archived [IDEA-017](../archived/idea-017-community-profiles-repository.md) (community profiles repository). Those define *how* a contribution physically reaches the community repo. This idea defines *what governs* a contribution once it arrives — the policy and metadata layer that has to exist before opening contributions to anyone outside the core team.

## Why this matters

Right now there is no policy attached to a community-shared profile. As soon as a third party submits one, several open questions become blockers at once:

- Under what license is the shared profile published, and how do contributors signal that they accept it?
- Whose name appears next to the profile, and how is that captured?
- What stops someone from putting slurs, ads, malware-flavoured strings, or unrelated political content in a free-text field and having it land in the public repo?
- How does someone who has never used Git contribute at all?

Each of these can be solved in isolation, but they interact — e.g. a "non-Git contribution path" via the app means the *app* becomes the place where the license is presented and accepted, which in turn shapes what the moderation step needs to check.

## Threads to work through

### 1. License — pick one, surface it everywhere it matters

Pick a single license for community-contributed profiles (and any other community-contributed artefacts: presets, wiring snippets, button-layout suggestions, content-page contributions). One license, no per-contribution choice.

Open questions:

- Which license? Candidates worth comparing: CC0, CC-BY 4.0, CC-BY-SA 4.0, MIT (for things that read more like config than creative work). The "right" answer depends on what we want downstream users to be able to do.
- Where must the license info appear?
  - In the community profiles repo root (LICENSE / NOTICE).
  - In the app's contribution flow (visible *before* submit, with explicit acceptance — not buried in a EULA).
  - In any web tool that exports a profile for sharing.
  - In the profile file itself? Or only at the repo level? (Per-file headers add noise; repo-level is typical for collections.)
- Does CONTRIBUTING.md need a Developer Certificate of Origin–style sign-off, or is the in-app accept-button sufficient?

### 2. Attribution — who shared it

Each contribution needs a contributor identifier. Open questions:

- What field(s)? Display name, GitHub-style handle, optional URL? Email is probably a no.
- Mandatory or optional? "Anonymous" should probably be allowed — but then we lose accountability for moderation.
- How do we render attribution? In the profile metadata, on a contributors page, both?
- Length limit on the display-name field — see thread 4.

### 3. Content moderation — automated screening of contributed text

We're a deliberately narrow technical project. Anything in a free-text field (profile name, description, attribution) should be on-topic for *configuring a footswitch pedal*. That gives us a sharper definition of "inappropriate" than a generic community would have.

Working definition (to be refined):

- Off-topic political/ideological content.
- Slurs, harassment, sexually explicit content.
- Advertising, spam, link-stuffing.
- Obvious obfuscation attempts (zalgo text, homoglyph attacks, control characters).
- Malware-flavoured payloads — even though profiles are data not code, paranoid scanning of free text is cheap.

Open questions:

- Build vs. buy: a wordlist + regex pre-commit check is easy and free; an LLM-based "is this on-topic for a footswitch pedal?" check is more flexible but adds API cost and a moving-target failure mode. Probably start with the cheap one.
- Where does the check run? Pre-commit hook on the community repo, CI check on PRs, *and* in-app preflight before the user even submits — pick which layers.
- How does a contributor see *why* their submission was rejected? "Your profile name contains a flagged word: `<word>`" beats a silent reject.
- Manual override path for the maintainer when the automated check has a false positive.

### 4. Free-text length limits

Cap every free-text field. Concrete proposals:

- Profile name: 60 chars.
- Profile description: 280 chars (Twitter-shaped — short enough to skim, long enough to explain).
- Contributor display name: 40 chars.
- Action-label / button-label fields: whatever the firmware/app already enforces (don't introduce a second limit here).

Limits should be enforced *both* in the app and in the repo-side validator, so a hand-edited PR can't sneak past the app's UI cap.

### 5. Internal log — who committed what, when, why

Beyond the visible attribution field, we may want an internal audit trail: contribution timestamp, source (app vN, web tool, hand-edited PR), moderation result, whether it was approved/edited/rejected by a maintainer.

Open questions:

- Is this just `git log` plus PR metadata, or a separate ledger file?
- Does the username-in-the-attribution-field already satisfy this, or do we want maintainer-only metadata that contributors can't see?
- Retention policy if we ever need to remove a contribution under a takedown request.

### 6. Low-friction onboarding — the non-tech-wizard path

The core tension: if contributions only come via GitHub PRs, we lose 90% of the people who would have something useful to share. If contributions come via a one-click app button, we inherit all of the moderation/attribution/license-acceptance burden in the app's UI.

Sub-threads:

- The in-app submit flow is the natural answer for *profile* contributions — IDEA-042 is the placeholder for that work. This idea should specify the metadata/policy bits that flow has to carry.
- For non-profile contributions (a how-to tip, a wiring photo, a translation), what's the path? GitHub Issues with a templated form? A web form that opens a PR on the contributor's behalf? Both have been done by other projects.
- "Maintainer reviews and merges" is fine while contribution volume is low. At higher volume the moderation queue becomes the bottleneck — flag this as a future concern, don't solve it now.

## What to decide first

Suggested ordering, since several of these block others:

1. Pick the license. Everything else builds on this — the in-app acceptance text, the CONTRIBUTING file, the repo NOTICE.
2. Decide what attribution metadata travels with a contribution (fields, mandatory/optional, length limits).
3. Define the moderation rules and pick the first-pass implementation (wordlist + length caps is probably enough to start).
4. *Then* design the in-app contribution UX (IDEA-042) on top of those decisions.
5. Internal-log shape and non-Git contribution paths can wait — they're refinements, not blockers.

## Out of scope (for this idea)

- The actual one-click submit flow — that's [IDEA-042](idea-042-one-click-profile-contribution-from-app.md).
- Code-of-conduct for maintainer ↔ contributor interaction. Adjacent topic, separate idea if/when needed.
- Localising the contribution UI — covered by [IDEA-052](idea-052-app-localization.md).
