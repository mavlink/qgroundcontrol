# Qt Quick Test — pre-send scan

Procedure for keeping user-facing chat messages free of
skill-internal references. Applies to every user-facing
message during a skill run, including questions asked via
clarification prompts.

Before sending, scan the draft for any of the following
tokens:

- `rule N` / `per rule N` / `rules N`
- `Step Nx` / `at Step N` / `workflow step N`
- `SKILL.md`
- `canonical template`
- `variant N` / `Variant N`
- `template variant`

Rewrite each offending sentence in plain English. The user
has not read this skill; references to its internals are
noise.

Example — instead of:

> Per rule 46, two items lack `objectName`...

write:

> Two items the test would exercise lack `objectName`, so
> `findChild` can't reach them — add them?

The substantive reason stands; the citation goes.

Internal reasoning (rule numbers, variant numbers, step
labels) is fine in your private thinking — it is the
user-facing text that must be scrubbed.
