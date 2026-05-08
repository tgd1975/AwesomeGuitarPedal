---
id: IDEA-050
title: Rule-grade memories belong in CLAUDE.md, not auto-memory
description: Most auto-memory `feedback_*` entries are rules misshelved out of `CLAUDE.md`. Codify the discipline (auto-memory = working notes only) and triage the existing entries.
category: 🛠️ tooling
---

Auto-memory has been collecting *rules* — the prescriptive kind that
should govern every session — when its actual job is to hold *working
notes*. The result is duplication, drift, and a bucket whose contents
no longer match its purpose.

This idea started as "version auto-memory like we version `CLAUDE.md`
and skills" (see [How this idea evolved](#how-this-idea-evolved)
below). On closer inspection that was the wrong proposal: it would
have committed Claude-authored content as durable artifacts without a
review gate, and it would have made auto-memory a peer of `CLAUDE.md`
— at which point the layer distinction collapses and you may as well
just write rules into `CLAUDE.md` directly. Which is exactly the
discipline this idea now proposes.

## The conceptual frame: three layers, three registers

`CLAUDE.md`, skill bodies, and auto-memory aren't three instances of
the same kind of artifact. They're three layers of a knowledge stack,
each with a different *register*:

| | `CLAUDE.md` / skills | Auto-memory |
|---|---|---|
| **Authored by** | The human, deliberately | Claude, opportunistically |
| **Authority** | Prescriptive — rules to follow | Descriptive — observations Claude inferred |
| **Reviewed before it exists** | Yes (you wrote it) | No (Claude wrote it) |
| **Audience** | Everyone working on the repo | Future-Claude reading past-Claude's notes |
| **Edit cadence** | Deliberate, rare | Continuous, routine |
| **Failure mode** | Wrong rule, deliberately written | Over-generalization from a single moment |

`CLAUDE.md` and skills are **codified intent**. Auto-memory is
**working notes**. Versioning the former makes sense (intent has
clear ownership and authority). Versioning the latter without a
review gate would convert provisional observations into durable
artifacts — which means they should have been rules in the first
place.

## Empirical finding: most auto-memory entries are mis-shelved

Sampled all 14 entries currently in
`~/.claude/projects/<hash>/memory/`:

| Category | Count | Examples | Right home |
|---|---|---|---|
| **Rules** disguised as feedback | 12 | `feedback_no_sudo_without_approval`, `feedback_close_task_after_ac_met`, `feedback_auto_activate_tasks`, `feedback_commit_staged_files`, `feedback_no_verify_after_three_checks`, `feedback_overview_regen_in_status_commits`, `feedback_commit_skill_no_external_git_add`, `feedback_no_auto_git_add_in_commit_wrapper`, `feedback_one_script_for_parsing`, `feedback_add_a_task_means_scaffold_only`, `feedback_idea_vs_task`, `feedback_generic_examples_in_ideas` | `CLAUDE.md`, or a skill body |
| **Profile facts** about the human | 1 | `project_owner_style` (AI-assisted dev model, no campaigns) | Auto-memory legitimately — shapes framing of suggestions, not a rule |
| **Transient state** | 1 | `project_instructables_decision` ("on hold pending another project") | Auto-memory legitimately — meant to evaporate when the trigger event happens |

So 12 of 14 entries are rules, and **the duplication is already
happening in practice**:

- `feedback_auto_activate_tasks` is mirrored to `CLAUDE.md` as the
  `## Auto-activate tasks when work begins` section.
- `feedback_commit_staged_files` overlaps the `## Commits go through
  /commit — always` section.
- `feedback_overview_regen_in_status_commits` overlaps the
  "OVERVIEW/EPICS/KANBAN are an exception" paragraph in `CLAUDE.md`.
- `/commit`'s `SKILL.md` codifies several more.

These were not deliberately duplicated. They were Claude-written rules
that the user later (correctly) promoted to `CLAUDE.md`, but the
auto-memory copy was never deleted. So the same rule lives in two or
three places simultaneously — noise, not redundancy.

## The discipline going forward

When Claude is about to save a `feedback_*` memory, the test is:

> Is this prescriptive — a rule that should apply to every session
> from now on?
>
> - **Yes** → it belongs in `CLAUDE.md` (or in a relevant `SKILL.md`
>   body if it's procedural). Propose the amendment to the human;
>   do not save it as auto-memory.
> - **No** — it's a descriptive observation, a profile fact, or
>   transient state → save it to auto-memory as today.

Auto-memory stays small, evaporates naturally, and stops drifting
into shadow `CLAUDE.md`. `CLAUDE.md` becomes the single source of
truth for rules — already-versioned, already-reviewable, already-
shared.

## Why the original "version auto-memory" framing was wrong

The original proposal in this file (preserved in git history) was to
mirror the auto-memory directory through `awesome-task-system/` and
have a sync script + pre-commit divergence check, by analogy with how
skills are mirrored. Three reasons that was wrong:

1. **Wrong analog.** Skills are human-authored on both sides of the
   mirror; the package copy is canonical and humans edit it. Auto-
   memory is Claude-authored, live, mid-session — the package
   pattern doesn't fit the writing model.
2. **No review gate.** Even with mirroring, every memory write would
   commit Claude's own output without prior human approval. That's
   acceptable for working notes (they're meant to be lossy and
   provisional) but unacceptable for durable, shared artifacts.
3. **Solves the wrong problem.** The real grievance ("commit that
   memory update" failing) is a symptom of the bucket mismatch, not
   of insufficient infrastructure. Once rules live in `CLAUDE.md`,
   the wish to commit them is satisfied by editing `CLAUDE.md` —
   which is already a normal commit.

A symlink alternative was considered (`memory/` redirected into the
repo). It's simpler than the mirror but has the same root flaw: it
collapses the rule/note layer distinction. Same diagnosis, same
rejection.

## How this idea evolved

For posterity (the discussion that produced the rewrite):

1. **Original framing** — "auto-memory is unversioned, that's a gap;
   apply the awesome-task-system mirror pattern."
2. **Conceptual question** — "we already version `CLAUDE.md` and
   skills, why not memory too?" Pushed the review past surface
   objections (multi-user, sync direction) and into the question of
   whether the three artifacts are actually the same kind of thing.
3. **Layer distinction surfaced** — codified intent vs working
   notes; the two have different authorship, authority, and review
   models. Versioning working notes naively collapses the
   distinction.
4. **Empirical check** — sampled the existing entries; found 12 of
   14 are rules, and several are already duplicated to `CLAUDE.md`.
   Confirmed the bucket mismatch is real and active.
5. **Reframe** — the gap isn't infrastructure. It's a sorting
   discipline: when Claude wants to save a rule, route it to
   `CLAUDE.md`, not to memory. Cleanup the existing mis-shelved
   entries.

## Task plan

Two tasks. Small enough that this idea is ready to promote.

### Task 1 — codify the routing discipline in `CLAUDE.md`

- Add a new section to `CLAUDE.md`, e.g. `## Auto-memory vs CLAUDE.md
  — what goes where`, defining the two registers (rules vs working
  notes) and the routing rule: prescriptive content goes into
  `CLAUDE.md` (or `SKILL.md` if procedural); descriptive observations,
  profile facts, and transient state stay in auto-memory.
- Include the test phrasing ("is this prescriptive — applies to every
  session from now on?") so the routing decision is unambiguous.
- Reference this idea (or its successor doc) for the rationale, so
  the section can stay short.

**Acceptance:** the next time Claude is about to save a `feedback_*`
memory, the section in `CLAUDE.md` is consulted and the entry either
becomes a `CLAUDE.md` amendment (proposed to Tobias) or is saved to
auto-memory as a non-rule.

### Task 2 — triage the existing 14 auto-memory entries

For each of the 12 rule-like `feedback_*` entries:

1. Search `CLAUDE.md` and the relevant `SKILL.md` files for an
   existing section that already covers the rule.
2. **If covered** — delete the auto-memory entry (and remove its line
   from `MEMORY.md`). It's redundant.
3. **If not covered** — draft a `CLAUDE.md` amendment that captures
   the rule. Get Tobias's approval. Land the amendment, then delete
   the auto-memory entry.

For the 2 `project_*` entries:

- `project_owner_style` — keep as-is. Verify: is it project-specific
  or about Tobias generally? If general, consider moving to user-
  global memory (`~/.claude/CLAUDE.md` if it exists, or
  user-global memory dir). Out of scope for this task otherwise.
- `project_instructables_decision` — keep as-is. It's transient and
  will evaporate when Tobias reports back from the other project.

**Acceptance:** after this task, `MEMORY.md` lists only entries that
are profile facts, transient state, or genuinely descriptive
observations. No `feedback_*` rule that is also (or should also be)
in `CLAUDE.md`. The duplication is gone.

### Out of scope

- No `awesome-task-system/memory/` package source.
- No `sync_memory.py` script.
- No pre-commit divergence check on the memory directory.
- No symlink redirect of the memory directory into the repo.
- No infrastructure changes whatsoever — only `CLAUDE.md` content +
  cleanup of existing memory entries.

## Related

- The original "version auto-memory infrastructure" framing is
  preserved in git history at the previous revision of this file.
- Sibling versioned-instruction surfaces:
  [CLAUDE.md](../../../../CLAUDE.md), the project instruction file;
  [`.claude/skills/`](../../../../.claude/skills/), the slash-command
  skill bodies; both already reviewable via normal PR flow.
- Auto-memory location:
  `~/.claude/projects/<project-hash>/memory/` — out of repo, per-
  machine, written by Claude during sessions. Stays that way under
  this proposal.
