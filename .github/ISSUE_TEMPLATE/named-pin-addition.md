---
name: Propose a standard pin name
about: Suggest adding a role to the named-pin v1 standard set (EPIC-029)
title: "[Named pin] "
labels: named-pins, needs-triage
assignees: ""
---

## Proposed name

`<lowercase_snake_case>` (e.g. `expression_1`, `bank_down`)

## Role description

One line: what this pin does in the builder's wiring.

## Why doesn't an existing name fit?

Look through [data/pin-names.schema.json](../../data/pin-names.schema.json)
and explain why none of the existing roles cover this case — or note
that the role is genuinely new.

## Builder context

At least one board build where this role exists. A short link to a
wiring photo / schematic / video helps, but is not required.

## Checklist

- [ ] Name is lowercase snake_case
- [ ] Role description fits in one line
- [ ] I have checked the existing v1 set for overlap
- [ ] This is a role I would actually use on a build, not a hypothetical

## Triage criteria

The maintainers' decision is one of:

- **Accept** — adds the name to the v1 enum in a follow-up PR.
- **Decline** — closed with a one-line reason (usually: overlaps an
  existing role).

The goal is lightweight, fast turnaround. This is acceptable
builder-scope friction; it is not a heavy review process.
