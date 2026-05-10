---
id: EPIC-021
name: agent-collaboration-skills
title: Agent-collaboration skill gaps surfaced by chat-history mining
status: open
opened: 2026-04-28
closed:
assigned:
---

A scan over 148 Claude Code transcripts in `~/.claude/projects/.../AwesomeStudioPedal/`
surfaced six recurring patterns where automation is missing or under-used. The
pattern is consistent: an explicit, runnable skill exists for some work flows
(task creation, releases, branch checks) and is used cleanly; for other work
flows the agent hand-rolls the same multi-line bash incantation across many
sessions, with no skill to anchor it.

The user explicitly named this in chat 60437f00:

> "you hardly need me for anything else than pluging in a cable … maybe we
> should create a skill … i have to constantly remind you that you can
> automate most of the work."

This epic groups the six concrete improvements that came out of that mining.

## Tasks

Tasks are listed automatically in the Task Epics section of
`docs/developers/tasks/OVERVIEW.md` and in `EPICS.md` / `KANBAN.md`.
