---
description: Multi-model review + synthesis + commit for the current branch
allowed-tools: Bash, Read, Write, Edit, Task
argument-hint: [base-branch]
---

# /review — sprig-ao

You are the **review orchestrator**. TDD-Guard already enforced the
red/green/refactor discipline at hook time. This command picks up at
Phase 5: independent review against `.orchestrator/review-rubric.md`,
synthesis, and commit.

`CLAUDE.md`, `AGENTS.md` (if present), `DESIGN.md` (if present),
`docs/CROSS_PLATFORM.md`, and `.orchestrator/review-rubric.md` are the
source of truth for project conventions and review scope. Defer to them.

## Inputs

- `$ARGUMENTS` — base branch (default: `origin/main`).
- Working tree: assume committed locally on the current branch. The command
  does **not** amend or rebase; it reviews, synthesizes, then commits the
  synthesis artifacts.

## Reviewer ground rules (high-weight zones — single-reviewer findings elevated)

1. Lifecycle state machine — `lifecycle-manager.ts`, `lifecycle-state.ts`,
   `deriveLegacyStatus`, terminal-reason enum
2. Session manager stale-runtime reconciliation — `sm.list()` in
   `packages/core/src/session-manager.ts`
3. Plugin contract — any exported interface in `packages/core/src/types.ts`
4. Cross-platform helpers — `packages/core/src/platform.ts`,
   `path-equality.ts`, Windows pty-host pipe protocol
5. Agent plugin `getActivityState` cascade — process check → JSONL
   actionable → native signal → JSONL fallback → null
6. Activity log contract — JSONL append-only, dedup, classification
7. SSE 5s interval — `useSessionEvents` hook
8. Worktree/state storage layout under `~/.agent-orchestrator/{hash}-{projectId}/`
9. Design tokens / dark theme / no inline styles
10. Agent plugin `setupWorkspaceHooks` — required for dashboard PR tracking

Full rubric in `.orchestrator/review-rubric.md`. Do not paraphrase it —
feed the file verbatim to each reviewer.

## Phase 5 — multi-model review

```bash
BASE="${ARGUMENTS:-origin/main}"
git diff "$BASE"...HEAD > .orchestrator/diff.patch
git diff --stat "$BASE"...HEAD > .orchestrator/diff.stat
```

Fan out three POV reviewers **in parallel** — issue all three calls in one
tool-use message. All reviewers are POV-only (findings, no fix proposals,
no synthesis). Synthesis happens in Phase 6.

1. **Claude review** via the `reviewer` subagent:
   ```
   Agent(subagent_type="reviewer", description="POV review against rubric",
         prompt="Review .orchestrator/diff.patch against
         .orchestrator/review-rubric.md, CLAUDE.md, and (if present)
         AGENTS.md, DESIGN.md, docs/CROSS_PLATFORM.md.
         List findings only, tagged BLOCKER/MAJOR/MINOR/NIT, grouped by
         file. Tag high-weight-zone hits with [HIGH-WEIGHT]. Do not
         propose fixes. Return the full markdown.")
   ```
   Save the assistant's verbatim output to `.orchestrator/review-claude.md`.

2. **Codex review** via the local Codex CLI:
   `codex exec --instruction "Review .orchestrator/diff.patch against
   .orchestrator/review-rubric.md, CLAUDE.md, and (if present) AGENTS.md,
   DESIGN.md, docs/CROSS_PLATFORM.md. List issues only with
   BLOCKER/MAJOR/MINOR/NIT tags, grouped by file. Tag high-weight-zone
   hits with [HIGH-WEIGHT]. Do not propose fixes."`
   → `.orchestrator/review-codex.md`.

3. **MiMo review** via the OpenRouter MCP server. Call
   `mcp__openrouter__chat_completion` with:
   - `model`: `xiaomi/mimo-v2.5-pro`
   - `temperature`: `0.2`
   - `max_tokens`: `16000` — MiMo v2.5-pro is a reasoning model; hidden
     reasoning tokens count against this budget, so do not lower it.
   - `messages`: a single `user` message whose content is the concatenation
     of `.orchestrator/review-rubric.md`, `CLAUDE.md`, any other
     source-of-truth docs, and `.orchestrator/diff.patch` (read all first
     and inline their text), prefaced by the same instruction text used
     for the Codex call above.

   Save the model's response verbatim to `.orchestrator/review-mimo.md`.
   If `mcp__openrouter__chat_completion` is not available, run
   `claude mcp list | grep openrouter` to confirm the server is registered,
   then ask the user to restart the session — do not silently skip MiMo.

4. **Gemini review** via the local `gemini` CLI (OAuth'd to a Gemini Pro
   account). Different training family from Claude/Codex/MiMo — primary
   reason it's in the panel.
   ```bash
   cat .orchestrator/review-rubric.md CLAUDE.md .orchestrator/diff.patch \
     | gemini -m gemini-3-pro-preview -p "You are a code reviewer. Below is a concatenation of the review rubric, CLAUDE.md, and the diff. Review the diff against the rubric and conventions. List findings ONLY, tagged BLOCKER/MAJOR/MINOR/NIT, grouped by file. Tag high-weight-zone hits with [HIGH-WEIGHT]. Do NOT propose fixes. Output the markdown findings only." \
     > .orchestrator/review-gemini.md
   ```
   If `gemini` is not on PATH, run `npm install -g @google/gemini-cli`
   then `gemini` once to OAuth. Do not silently skip Gemini.

## Phase 6 — synthesis (synthesizer subagent)

Dispatch the `synthesizer` subagent to merge the 2–N reviewer files:

```
Agent(subagent_type="synthesizer", description="Merge review POVs",
      prompt="Read every .orchestrator/review-*.md (excluding
      review-synthesis.md and review-rubric.md), apply the consensus
      rules from your system prompt, and write
      .orchestrator/review-synthesis.md.")
```

The synthesizer reads the reviewer outputs only — it never reads the diff.
It applies these rules:

- **Consensus** (≥2 reviewers): always surface, highest severity wins.
- **Single-reviewer in high-weight zone**: elevate.
- **Single-reviewer outside high-weight zones**: surface only if reasoning
  is sound; suppress pure style nits.
- **Disagreements**: surface both positions verbatim; user decides.

If only one `review-*.md` file is present, synthesizer halts. The invoker
(`/review`) decides whether to ship the single POV as-is or re-run the
fan-out.

**Do not auto-apply fixes.** Each fix is a fresh trip through the workflow
(new test → green → /review again).

## Phase 7 — commit

Only after the user approves `.orchestrator/review-synthesis.md`:

```bash
git add .orchestrator/diff.patch .orchestrator/diff.stat \
        .orchestrator/review-claude.md .orchestrator/review-codex.md \
        .orchestrator/review-mimo.md .orchestrator/review-gemini.md \
        .orchestrator/review-synthesis.md
git commit -m "chore(review): record review artifacts for $(git rev-parse --short HEAD)"
```

PR description must include:
- The verification command for the change under review (e.g. `pnpm test`,
  `pnpm --filter @aoagents/ao-core test`).
- A summary of `.orchestrator/review-synthesis.md`.
- The user merges manually — no auto-merge.

## Quick reference

```
Phase 5 REVIEW    → parallel POV fan-out: reviewer subagent / codex exec / openrouter MCP (MiMo) / gemini CLI
Phase 6 SYNTHESIS → orchestrator merges reviews into .orchestrator/review-synthesis.md
Phase 7 COMMIT    → record artifacts; PR with verification command + synthesis summary
```
