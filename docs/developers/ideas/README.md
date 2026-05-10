# Ideas — folder layout and naming

Ideas are lightweight, qualitative proposals. Each lives in
[`open/`](open/) until it is converted into a task or moved to
[`archived/`](archived/) for history.

## File naming

| Pattern | Role | Frontmatter | In OVERVIEW |
|---|---|---|---|
| `idea-NNN-<slug>.md` | Main idea file (one per IDEA-NNN) | Yes — `id: IDEA-NNN`, `title:`, `category:`, optional `description:` | Yes — one row |
| `idea-NNN.<sub-slug>.md` | Sub-note attached to the main idea | No frontmatter (or only a one-line "sub-note of IDEA-NNN" header) | No |

The separator between `NNN` and the slug carries the meaning:

- **Hyphen** (`idea-027-circuit-skill.md`) → main file. Renders in
  [OVERVIEW.md](OVERVIEW.md) as one row.
- **Dot** (`idea-027.erc-engine.md`) → sub-note. Never renders in
  the index. Linked from the main file's body.

The rule is grep-able: `idea-027-*` matches only the main file,
`idea-027.*` matches only sub-files. Alphabetical sort still groups
them together in the folder listing.

## When to use sub-files

Use a sub-file when an idea has substantial supporting material that
would bloat the main file: a separate concept exploration, a
discarded alternative kept for history, a deep technical sub-design,
or per-component breakdowns. Each sub-file gets a short title at the
top and is linked from the main file's body.

If a "sub-idea" is genuinely a different proposal that could ship
independently, give it its own `IDEA-NNN` instead of attaching it as
a sub-file. Sub-files are *notes about the same idea*, not separate
ideas sharing a number.

## Example

```
open/
  idea-027-circuit-skill.md          # main — frontmatter, in OVERVIEW
  idea-027.components.md             # sub-note: component catalog
  idea-027.erc-engine.md             # sub-note: ERC design
  idea-027.layout-engine-concept.md  # sub-note: layout approach
  idea-027.yaml-format.md            # sub-note: file format
```

The main file (`idea-027-circuit-skill.md`) links to each sub-file
in its body so readers can navigate the full set.

## Scaffolding new ideas

Use [`/ts-idea-new`](../../../.claude/skills/ts-idea-new/SKILL.md)
to scaffold the main file. Sub-files are created by hand — the skill
deliberately does not scaffold them, since deciding *whether* a piece
of content deserves its own sub-file is an editorial call.
