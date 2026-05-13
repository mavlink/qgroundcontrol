---
description: Orchestrate a multi-slice task by dispatching Codex per slice with RED/GREEN gates and per-slice verification. Architect-mode (no production-code edits by Claude Code).
allowed-tools: Bash, Read, Write, Edit, Task, Agent
argument-hint: <plan-path-or-task-description>
---

# /orchestrate — qgroundcontrol

You are the **orchestrator** in Architect mode. The executor (`jcode`,
sometimes Codex) writes production code; you plan, dispatch, verify,
aggregate. You **never** edit `src/**`, root or per-module `CMakeLists.txt`,
`cmake/**`, `.clang-format`, `.clang-tidy`, `.pre-commit-config.yaml`,
`.github/workflows/**`, or `.github/actions/**` directly. All such edits route
through the executor — either a direct `jcode` Bash dispatch (default) or, for
rescue cases, `Agent(subagent_type="codex:codex-rescue", ...)`.

`CLAUDE.md`, `AGENTS.md`, `CODING_STYLE.md`, `test/TESTING.md`,
`.pre-commit-config.yaml`, `.orchestrator/templates/*.md`, and any plan in
`.orchestrator/plans/` are the source of truth. Defer to them.

## Inputs

`$ARGUMENTS` is one of:
- A plan path: `.orchestrator/plans/03-mission-upload-refactor.md` → drive that plan.
- A free-form task: `"Add a per-vehicle telemetry rate setting"` →
  draft a slice plan first, write it to `.orchestrator/work/<slug>/plan.md`,
  then drive it.

## Token-budget directive (always)

The main `/orchestrate` session is Opus. To minimise Opus token usage,
**every invocation must delegate planning to the architect subagent**
(`.claude/agents/architect.md`, runs on Sonnet) regardless of how the task
is phrased. Do this on the very first turn, before any other tool call
except a read-only `git status --short` to verify a clean tree:

```
Agent(subagent_type="architect", description="Plan: <task>",
      prompt="<verbatim $ARGUMENTS plus a note that the plan must follow
               .orchestrator/templates/* and mark each slice's type>")
```

Wait for the architect's plan, then resume Step 1 below using that plan as
input. Do not re-plan, re-draft, or re-clarify in the main session — your
job from that point is dispatch + verification, not planning.

Exception: if `$ARGUMENTS` already points at an existing
`.orchestrator/plans/NN-*.md` AND the file already contains slice-type
headers, you may skip the architect dispatch and consume the file
directly.

## Workflow

### Step 1 — resolve task & state

Compute `slug = kebab(task name)`. Create `.orchestrator/work/<slug>/` if
missing. Inside:

- `plan.md` — the slice plan (copied from `.orchestrator/plans/NN-*.md` or
  drafted from the free-form task).
- `state.md` — per-task state, append-only log. Bootstrap from
  `.orchestrator/templates/state-template.md` if absent.
- `slice-N-brief.md` — one per slice, written before each Codex dispatch.
- `slice-N-result.md` — Codex's verbatim return for the slice.
- `slice-N-diff.patch` — `git diff` snapshot after each Codex dispatch.

If `state.md` already exists with incomplete slices, **resume** from the
first incomplete slice. Do NOT re-run completed slices.

### Step 2 — read each slice's declared type

The architect subagent (`.claude/agents/architect.md`) marks every slice
with its type:

- **TDD slice** — two dispatches: RED then GREEN.
- **Refactor slice** — one dispatch.
- **Doc/config slice** — orchestrator-scope (docs, config files I'm allowed
  to edit). I do these inline; no Codex dispatch.

If the architect produced the plan, the type is in the slice header — just
read it. If a legacy plan predates the architect subagent and a slice type
is missing, halt and ask the user to re-classify rather than inferring.

### Step 3 — per-slice loop

For slice N:

#### 3a. Verify clean working tree

`git status --short` must be empty. If not, halt and report — never proceed
with dirty working state.

#### 3b. Write the brief

The architect's plan already contains the slot-fillable structure (behavior
contract, test cases, files touched, slice acceptance criteria). Copy the
right template from `.orchestrator/templates/`:

- TDD RED phase → `red-brief-template.md`
- TDD GREEN phase → `green-brief-template.md`
- Refactor → `refactor-brief-template.md`

Paste the plan's slice fields into the template slots — no re-interpretation.
Save to `.orchestrator/work/<slug>/slice-N-{red,green,refactor}-brief.md`.

#### 3c. Dispatch the executor (jcode)

Default path — plan-driven slice with a pre-built brief at
`.orchestrator/briefs/<slug>.md`. Dispatch `jcode` directly via `Bash` (NOT
via Agent/Task):

```bash
jcode run -C /Users/stratten/Documents/Github/qgroundcontrol -p auto "$(cat .orchestrator/briefs/<slug>.md)"
```

`-C` / `--cwd` sets the working directory, `-p` / `--provider auto` picks the
provider automatically, and the brief is passed as the positional `<MESSAGE>`
argument. **Do not use `-m`** — `-m` is `--model`, not `--message`.

Rescue path — if `jcode` returns an empty diff or a failure and the brief
itself needs tightening, dispatch the Codex rescue subagent (NOT
`codex-handoff` as a first-line; that is a documented fallback path):

```
Agent(subagent_type="codex:codex-rescue", description="Slice N rescue: <short>", prompt="<sharpened brief>")
```

The Codex companion runs in `--write` mode by default and edits files
directly. Reserve `codex-handoff` for the rare case where you want the
architect→Codex pipeline explicitly (e.g., to compare jcode vs Codex output
on a contract slice).

#### 3d. Read return + diff

Save the executor's stdout verbatim to `slice-N-result.md`. Snapshot diff:

```bash
git diff > .orchestrator/work/<slug>/slice-N-diff.patch
git diff --stat > .orchestrator/work/<slug>/slice-N-diff.stat
```

#### 3e. Verify

Use the slice's declared verification command from the brief. `build/` must
already be configured with
`cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`. Typical commands:

- Subsystem unit: `cd build && ctest --output-on-failure -L Unit -R <SubsystemRegex> --parallel $(nproc)`
- All unit: `cd build && ctest --output-on-failure -L Unit --parallel $(nproc)`
- Integration: `cd build && ctest --output-on-failure -L Integration --parallel $(nproc)`
- Build (acts as C++ type check): `cmake --build build --config Release --parallel`
- Lint: `pre-commit run --all-files`

For TDD RED phase:
- Run the verification command — must show **the new test failing for the
  right reason** (not a build error that blocks the whole suite). Inspect
  QTest / ctest output.
- If green or wrong-reason red, **reject**: dispatch a follow-up via
  `Agent(subagent_type="codex:codex-rescue", ...)` with the failure context.

For TDD GREEN phase:
- Run the verification command — must show **all tests green**, including
  the previously failing one.
- Run `cmake --build build --config Release --parallel` if headers changed
  (the build is the C++ type check). `pre-commit run --all-files` if the
  surface is non-trivial or touches files matched by clang-format /
  clang-tidy / clazy / qmllint / vehicle-null-check / check-no-qassert.
- If anything red, dispatch executor follow-up with the failure context.

For refactor slice:
- Run the verification command — must show **same test count, all green**.
- Compare test ids against pre-slice baseline if behavior was supposed to
  be unchanged. No silent test loss.
- If the slice touched QML: also run `pre-commit run qmllint --files <paths>`.

#### 3f. Update state

Append to `state.md` (don't overwrite). Mark slice N done with timestamp,
diff stat, test result.

#### 3g. Commit (orchestrator does this directly via Bash, NOT via the executor)

The orchestrator is allowed to run `git add` and `git commit` on the
changes the executor produced — this is bookkeeping, not production-code
editing. Commit message format:

```
<type>(<scope>): <slice description> - Refs <plan>

Slice N of plan <path>. Dispatched via /orchestrate.
```

`<type>` is conventional (`refactor`, `feat`, `fix`, `test`, `chore`).
`<scope>` matches the plan's affected subsystem (e.g., `vehicle`,
`factsystem`, `firmwareplugin-px4`, `mission`, `mavlink`, `qmlcontrols`,
`tools`).

DO NOT push. Pushing is a separate manual step.

### Step 4 — at PR boundary

After the last slice in the current PR completes:

1. Build the diff: `git diff origin/main...HEAD > .orchestrator/diff.patch`
   and `git diff --stat origin/main...HEAD > .orchestrator/diff.stat`.
2. Invoke `/review` for multi-model review.
3. Halt — surface synthesis to user. Do NOT auto-fix findings.

### Step 5 — handoff

Tell the user:
- Slices completed (count, list).
- Total LOC added/removed.
- Final test result.
- Path to synthesis (if review ran).
- Any rejected dispatches and why.

### Step 6 — harvest & clean (Phase 8)

A slice is not done until its harvest is recorded. Run after Step 5.

#### 6a. Update STATUS.md — move slice to "Recently Merged"

If STATUS.md exists in the repo root, move the slice row out of **In
Flight** (or **Blocked**) into **Recently Merged**, fill in the merge date
(today, ISO) and a placeholder Harvest cell (filled in 6c). Drop any rows
in **Recently Merged** older than 7 days. If STATUS.md does not exist,
skip this sub-step.

#### 6b. Identify harvestable knowledge

Read, in this order: the slice plan, `.orchestrator/review-synthesis.md` if
present, and the slice's commit messages from `git log --format=%B <range>`.

For each candidate fact, classify against the rubric below. **Default is
discard** — only fold in if it clearly meets one of the durable criteria.

| Discovery | Goes to | Only if… |
|-----------|---------|----------|
| New project convention (naming, file layout, error pattern) | `AGENTS.md` or `CODING_STYLE.md` (relevant section) | a future slice in *any* subsystem would benefit |
| Architectural decision or load-bearing constraint (Fact contract, FirmwarePlugin interface, MultiVehicle invariant, cross-platform CI helper) | `AGENTS.md` (Golden Rules) or the relevant `docs/` file | the constraint will outlive the current sprint and could be violated by future agents |
| Recurring bug pattern or review check | `.orchestrator/review-rubric.md` | the same pattern has been flagged in **2+ slices**; on first sighting, leave it in the synthesis only |
| New verification command or test pattern | one-line note in AGENTS.md "Build & Test Commands" section | the command will be re-run |
| Non-obvious quirk (Qt version footgun, MAVLink library quirk, platform CI trap) | `AGENTS.md` → `## Known quirks` (create if absent) | the quirk is non-obvious from the code itself |
| Plan-specific implementation detail (which function, which file, which test) | **discard** | git log + the merged code already document it |
| One-off bugfix where the fix *is* the documentation | **discard** | the commit message captures it |

If no facts pass the filter, log "no harvestable knowledge — skip" in the
slice's `state.md` and proceed to 6d.

#### 6c. Propose harvest, get approval, fold

For each surviving fact, write a 1–3 sentence summary of what would be added
and where. Present the full list to the user as a single message and wait
for explicit approval. On `y`, perform the edits via `Edit` tool calls
(orchestrator-scope; no executor dispatch). On `edit`, take the user's
revisions verbatim. On `n`, skip the fold and record "user declined" in
`state.md`.

#### 6d. Clean ephemeral docs

Per project convention, `.orchestrator/work/` is **tracked**
(not gitignored), so other devs can see in-flight runs. After harvest,
delete the slice's scratch via git:

- `git rm -r .orchestrator/work/<slug>/`
- `git rm .orchestrator/plans/<id>-*.md`
- `git rm .orchestrator/review-synthesis.md` (if dated to this slice)
- `git rm .orchestrator/review-*.md` (per-reviewer files from this slice)
- `git rm .orchestrator/diff.patch .orchestrator/diff.stat` (if present)

Do **not** delete `.orchestrator/review-rubric.md`,
`.orchestrator/templates/`, `.orchestrator/work/README.md`, or `STATUS.md`.

Commit the harvest + cleanup as a single follow-up commit:

```
chore(harvest): fold <id> into docs and clean ephemeral

- CLAUDE.md: <1-line>
- delete: .orchestrator/plans/<id>-*.md, .orchestrator/work/<slug>/

Refs <id>
```

#### 6e. Linear-sourced slices: comment back

If the slice's Source is a Linear ticket, post a comment via
`mcp__claude_ai_Linear__save_comment` with the merge commit link and a
one-line harvest summary, then mark the issue done if not already.

## Failure handling

| Scenario | Action |
|----------|--------|
| Executor returns with no diff | Re-dispatch with sharpened brief; if 2 rejections in a row, escalate to `Agent(subagent_type="codex:codex-rescue", ...)`; if that also fails, halt and ask user. |
| Executor edits files outside the slice's allowed scope | Restore ONLY the paths the slice was authorised to touch — read the "Files touched" table from the slice plan/brief and run `git checkout HEAD -- <path1> <path2> ...` per-path. Do NOT run a bulk `git checkout` against the whole worktree (destructive: discards unrelated workspace state). If the authorised paths are not recorded, STOP and prompt the user. After restore, update brief with stricter scope, re-dispatch once. |
| Tests fail after GREEN dispatch | Dispatch executor follow-up with the failing test output; max 2 follow-ups per slice. |
| Working tree dirty at slice start | Halt. Never run an orchestration over uncommitted changes. |
| User overrides the dirty-tree halt ("roll it in") | Before dispatch, snapshot the pre-slice tree: `git stash push -u -m pre-slice-<slug> && git stash show -p stash@{0} > .orchestrator/work/<slug>/pre-slice.patch && git stash pop`. After the slice commits, diff `git status` against the original dirty set; if any pre-slice-dirty path is no longer dirty AND isn't in the committed scope, the executor silently reverted it — re-apply its chunk from `pre-slice.patch`. |
| Plan slice underspecified | Read the source files (read-only), draft a sharper brief, save back to slice-N-brief.md, dispatch. |

## What the orchestrator does NOT do

- Edit `src/**`, root or per-module `CMakeLists.txt`, `cmake/**`,
  `.clang-format`, `.clang-tidy`, `.pre-commit-config.yaml`,
  `.github/workflows/**`, `.github/actions/**`, or `tools/pyproject.toml`.
- Push to remote.
- Merge PRs.
- Auto-fix `/review` findings (each fix is a fresh `/orchestrate` call).
- Run the executor on read-only investigation tasks (use `Agent(subagent_type="Explore")` for that).
