---
description: Harvest durable knowledge from a completed slice into main docs and clean ephemeral plan/review files.
allowed-tools: Bash, Read, Write, Edit, Glob, Grep
argument-hint: <slice-id> [--dry-run]
---

# /harvest — fold one slice into the main docs and clean its scratch

You are running the same Phase 8 logic that `/orchestrate` ends with, but
standalone — for slices that were completed outside the full orchestrator
flow (plan-skip bugfixes, hand-driven small changes).

`$ARGUMENTS` is `<slice-id> [--dry-run]`.

## Process

1. **Locate the slice.** Search `.orchestrator/plans/` for a file matching
   `*<slice-id>*.md`. Search `.orchestrator/work/` for a matching `<slug>/`.
   Search recent `git log --grep "<slice-id>" --format="%H %s"` for the
   merge commit. If nothing matches, halt and ask the user for the merge
   commit SHA.
2. **Verify clean tree.** `git status --short` must be empty.
3. **Run Phase 8 sub-steps 6a–6d** from `/orchestrate`. Skip 6e (Linear
   comment) unless the slice's plan or STATUS.md row identifies a Linear
   source.
4. **`--dry-run` mode.** Print:
   - The proposed STATUS.md row movement (before/after diff), if STATUS.md exists.
   - For each main-doc target, a unified diff of the proposed insert.
   - The full list of files that would be deleted (`ls -la` against the
     glob).
   Then **stop without writing or deleting anything**. Do not commit.
5. **Default mode.** Execute the proposal exactly as `/orchestrate` Step 6
   would, including the user-approval prompt at sub-step 6c.

## What `/harvest` does NOT do

- Run tests. The slice is assumed merged; harvest is doc work only.
- Touch `packages/*/src/`, `package.json`, `pnpm-lock.yaml`, `tsconfig*.json`,
  `eslint.config.js`, or `.github/workflows/`. Same scope as the architect.
- Push or merge anything.
- Re-harvest a slice. If the slice's plan/work dir is already gone and
  STATUS shows it under "Recently Merged" with a Harvest summary, halt and
  report "already harvested".
