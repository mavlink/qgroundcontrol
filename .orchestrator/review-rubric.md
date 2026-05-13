# sprig-ao Review Rubric

Use this rubric in Phase 5 of the orchestrator workflow. Reviewers (Claude,
Codex, MiMo, Gemini) read this alongside the diff and `CLAUDE.md` and produce
a structured issue list. **Findings under "high-weight zones" are elevated in
synthesis even when only one reviewer flags them.**

Severity tags reviewers should use:
- **BLOCKER** — must fix before merge; would corrupt session/runtime state,
  break Windows users, violate a plugin contract, deadlock the lifecycle state
  machine, leak credentials, or regress pinned behavior
- **MAJOR** — should fix before merge; real bug, semantic gap, missing test
  for new behavior, plugin contract drift not captured in `types.ts`
- **MINOR** — nice to fix; no functional impact
- **NIT** — style, naming, organization

Suppress NITs in synthesis unless multiple reviewers agree on the same one.

---

## High-weight zones (single-reviewer findings elevated)

These are paths where a single sharp reviewer catching an issue is enough to
act. Two LLMs trained on similar data will hallucinate the same wrong answer
here, so single hits matter.

1. **Lifecycle state machine** — `packages/core/src/lifecycle-manager.ts`,
   `lifecycle-state.ts`, `deriveLegacyStatus`, the canonical state and
   terminal-reason enums. Implicit transition dependencies; small changes
   break dashboard status.
2. **Session manager stale-runtime reconciliation** —
   `packages/core/src/session-manager.ts`, especially `sm.list()`. The
   *only* place dead runtimes get persisted as `runtime_lost`.
3. **Plugin contract** — any exported interface in
   `packages/core/src/types.ts`. Changing a shape breaks every plugin.
4. **Cross-platform helpers** — `packages/core/src/platform.ts`,
   `path-equality.ts`, Windows pty-host pipe protocol, signal/PID probes.
   See `docs/CROSS_PLATFORM.md` for the full helper inventory.
5. **Agent plugin `getActivityState` cascade** — process check → JSONL
   actionable state → native signal → JSONL entry fallback → null.
   Skipping the fallback breaks the dashboard activity display.
6. **Activity log contract** — JSONL append-only, dedup window,
   classification helpers (`classifyTerminalActivity`, `recordTerminalActivity`).
7. **SSE 5s interval** — `useSessionEvents` hook. CLAUDE.md C-14 invariant.
8. **Worktree / state storage layout** —
   `~/.agent-orchestrator/{hash}-{projectId}/` where hash is SHA-256 of the
   config directory. Collision prevention across checkouts.
9. **Design tokens / dark theme** — `packages/web/src/app/globals.css`,
   Tailwind v4 `@theme` block. CLAUDE.md C-02 (no inline styles), C-05
   (dark theme preserved), C-01 (no new UI libs).
10. **Agent plugin `setupWorkspaceHooks`** — without it, PRs created by
    agents don't appear in the dashboard. Two patterns: agent-native hooks
    (Claude Code PostToolUse) and PATH wrappers (Codex/Aider/OpenCode
    `~/.ao/bin/gh` and `~/.ao/bin/git`).

---

## 1. Core types and plugin contract (`packages/core/src/types.ts`)

### Interface stability
- [ ] Does the diff change an exported interface? If yes, are ALL implementing
      plugins updated in the same PR (or is there a documented migration plan)?
- [ ] New optional fields are fine. Required fields, renamed fields, or
      removed fields are BLOCKER unless all implementations are updated.
- [ ] Does the change introduce a new plugin slot? If yes, is the slot
      documented in CLAUDE.md "Plugin System (8 Slots)" and is at least one
      default implementation present?

### Imports
- [ ] No barrel files added (except the canonical `core/src/index.ts`)?
- [ ] `import type` used for type-only imports (enforced by ESLint)?

---

## 2. Lifecycle state machine (`packages/core/src/lifecycle-*.ts`)

### Canonical states & terminal reasons
- [ ] Any new state in `LifecycleState` mapped in `deriveLegacyStatus`?
- [ ] Any new terminal reason added to the `TerminalReason` enum AND mapped
      to a legacy status in `deriveLegacyStatus`?
- [ ] `isTerminalSession` updated if a new terminal reason was added?

### Transitions
- [ ] State transitions documented (in code comments or the plan)?
- [ ] Does the change preserve the invariant that `sm.list()` is the only
      place `runtime_lost` is persisted to disk?
- [ ] Polling-loop reactions don't double-fire on the same state change?

### Storage
- [ ] Session metadata written under the canonical
      `~/.agent-orchestrator/{hash}-{projectId}/sessions/{sessionId}` path?
- [ ] No hardcoded paths that bypass the hash-prefix scheme (collision risk
      across multiple checkouts)?

---

## 3. Session manager (`packages/core/src/session-manager.ts`)

### Stale runtime detection
- [ ] If the diff touches `sm.list()` or enrichment, does it still detect dead
      runtimes (tmux/process gone) and persist `runtime_lost` to disk?
- [ ] No "active" status returned for sessions with dead runtimes?

### CRUD invariants
- [ ] Session ID generation collision-free (UUID, monotonic, or
      hash-prefixed)?
- [ ] Archives written to `archive/{sessionId}_{timestamp}` per the storage
      contract?

---

## 4. Cross-platform (`docs/CROSS_PLATFORM.md` is normative)

### The Golden Rule
- [ ] **No new `process.platform === "win32"` checks inlined.** Use
      `isWindows()` from `@aoagents/ao-core` or add a helper to
      `packages/core/src/platform.ts`. This is a BLOCKER even for one-liners.

### Process and shell
- [ ] Process spawn uses `getShell()` (PowerShell on Windows, `/bin/sh`
      elsewhere) or explicit `shell: isWindows()` for `.cmd`/`.bat`/`.exe`
      shim resolution?
- [ ] Process kill uses `killProcessTree(pid, signal?)` not bare
      `process.kill()` (which doesn't reach descendants on POSIX or anywhere
      on Windows)?
- [ ] Signal-0 probe handles **both** EPERM (process exists, owned by another
      user) and ESRCH (process gone)? Treating EPERM as "gone" causes false
      terminations on Windows.

### Paths
- [ ] Path comparisons use `pathsEqual()` / `canonicalCompareKey()` (NTFS
      and APFS are case-insensitive; naive `===` breaks on Windows/macOS)?
- [ ] No `/dev/null`, no `$(cat …)`, no POSIX-only shell idioms in
      cross-platform code paths?
- [ ] PowerShell-vs-bash quoting: shell args escaped via `shellEscape()`
      (which does the right thing per platform)?

### Network
- [ ] No bare `localhost` for binds that might resolve to IPv6 on Windows
      and stall? Use `127.0.0.1` explicitly or the project's resolved
      helper.

### Windows pty-host (runtime-process plugin)
- [ ] Named pipe path resolved via `getPipePath()` / `resolvePipePath()`?
- [ ] Pty-host registered via `registerWindowsPtyHost` and unregistered on
      teardown?
- [ ] `sweepWindowsPtyHosts()` invoked during `ao stop` cleanup so orphan
      hosts are reaped?

---

## 5. Agent plugins (`packages/plugins/agent-*`)

### `Agent` interface completeness
- [ ] All required methods implemented: `getLaunchCommand`,
      `getEnvironment`, `detectActivity`, `getActivityState`,
      `isProcessRunning`, `getSessionInfo`?
- [ ] Optional methods implemented where applicable: `getRestoreCommand`,
      `setupWorkspaceHooks`, `postLaunchSetup`, `recordActivity`?
- [ ] **`setupWorkspaceHooks` is present**? Without it, PRs created by this
      agent will not appear in the dashboard.

### `getActivityState` cascade (BLOCKER if any step missing)
- [ ] **Step 1 — process check:** if not running, return
      `{ state: "exited", timestamp }`.
- [ ] **Step 2 — actionable states:** read `waiting_input` / `blocked` via
      `checkActivityLogState(activityResult)` from native JSONL OR AO
      activity JSONL; if found, return.
- [ ] **Step 3 — native signal (if applicable):** classify by age (active
      <30s / ready 30s–threshold / idle >threshold).
- [ ] **Step 4 — JSONL entry fallback (REQUIRED):** call
      `getActivityFallbackState(activityResult, activeWindowMs, threshold)`.
      Skipping this returns `null` whenever the native signal fails and the
      dashboard goes blind. This was a real bug in the OpenCode plugin.
- [ ] Returns `null` only if there is genuinely no data at all.

### `isProcessRunning`
- [ ] Supports tmux runtime (TTY-based `ps` lookup with process-name regex)?
- [ ] Supports process runtime (PID signal-0 with EPERM handling)?
- [ ] Regex matches both the node wrapper name AND the actual binary
      (some agents install as `.agentname` with a dot prefix)?
- [ ] Returns `false` (not `null`) on error?

### PATH wrappers
- [ ] If this agent uses PATH wrappers (Codex, Aider, OpenCode), is
      `setupPathWrapperWorkspace(workspacePath)` invoked?
- [ ] `~/.ao/bin` prepended to PATH via `buildAgentPath(basePath?)`?

### Required tests
- [ ] Returns `exited` when process is not running
- [ ] Returns `waiting_input` from JSONL when at a permission prompt
- [ ] Returns `blocked` from JSONL when at an error state
- [ ] Returns `active` from native signal (if applicable)
- [ ] Returns `active` from JSONL entry fallback when native signal fails (fresh entry)
- [ ] Returns `idle` from JSONL entry fallback when native signal fails (old entry)
- [ ] Returns `null` when both native and JSONL are unavailable

---

## 6. Runtime plugins (`packages/plugins/runtime-*`)

### tmux runtime
- [ ] Session names scoped by `sessionPrefix` to avoid collisions across
      checkouts (see issue #1705 pattern)?
- [ ] PTY cleanup on `destroy()` — no leaked slots on macOS (#1710, #1718)?

### Process runtime (Windows-heavy)
- [ ] Pty-host registry entries cleaned up on session terminate?
- [ ] `validateSessionId()` from `@/server/tmux-utils` called before any
      pipe/shell use to prevent injection?
- [ ] `connectPtyHost` failures handled (host died, pipe path stale)?

---

## 7. Web dashboard (`packages/web/`)

### Tailwind & styling
- [ ] **No inline `style=` attributes** (C-02). Use Tailwind utility classes
      and `var(--color-*)` design tokens from `globals.css`.
- [ ] No new UI component libraries (C-01) — no Radix, no shadcn, no Headless UI.
- [ ] Dark theme preserved (C-05). Don't add light-mode-only colors.
- [ ] Tokens used via `var(--color-*)` not hex literals where a token exists.

### Components
- [ ] Component file ≤ 400 lines (C-04)? If over, split.
- [ ] Client components marked `"use client"` only when they actually need
      client-side state or effects?
- [ ] Test file present for any new component (C-12)?

### Data flow
- [ ] SSE 5s interval preserved (C-14) — `useSessionEvents` polling
      interval is load-bearing for dashboard latency expectations.
- [ ] No new state libraries (no Redux, no Zustand). React hooks only.
- [ ] Sidebar consumers use unscoped `useSessionEvents` (no project filter)
      so cross-project sessions show up?

### Hooks
- [ ] Custom hooks named `use*` and live in `hooks/`?
- [ ] Boolean variables prefixed `is`/`has`?

---

## 8. CLI (`packages/cli/`)

### `ao start` / `ao stop` invariants
- [ ] `ao stop` (no args) kills ALL sessions across ALL projects (loads
      global config, not just local)?
- [ ] `ao stop <project>` kills only that project's sessions, does NOT
      kill the parent dashboard?
- [ ] `ao start` Ctrl+C does a full graceful shutdown via the same path as
      `ao stop` (with 10s hard timeout)?
- [ ] `LastStopState` includes `otherProjects` for cross-project session
      restore?
- [ ] Tab completions merge local config + global config?

### Config loading
- [ ] `loadConfig()` searches up from cwd for `agent-orchestrator.yaml`?
- [ ] Cross-project visibility commands fall back to global
      `~/.agent-orchestrator/config.yaml`?

---

## 9. Plugin standards (`packages/plugins/*`)

### Manifest & exports
- [ ] Package named `@aoagents/ao-plugin-{slot}-{name}` (lowercase, hyphenated)?
- [ ] `manifest.name` matches the `{name}` suffix?
- [ ] `manifest.slot` uses `as const`?
- [ ] Default export is `{ manifest, create, detect } satisfies PluginModule<T>`?

### Config & error handling
- [ ] Plugin-level config validated once in `create()` and closured?
- [ ] Errors wrapped with `cause`: `throw new Error("msg", { cause: err })`?
- [ ] Returns `null` for "not found", throws for unexpected errors?
- [ ] `shellEscape()` used for all command arguments?

### Testing
- [ ] `src/__tests__/index.test.ts` present with manifest values, `create()`
      shape, all public methods, and error paths covered?

---

## 10. Security / safety

- [ ] No hardcoded credentials, API keys, PII?
- [ ] Shell args escaped via `shellEscape()`?
- [ ] User-supplied session IDs validated via `validateSessionId()`?
- [ ] Webhook URLs validated via `validateUrl()`?
- [ ] No `.env` or secret-bearing files committed (gitleaks should catch,
      but reviewer eyes too)?

---

## 11. Tests

### Discipline
- [ ] Vitest tests in `__tests__/` subdirs, named `*.test.ts` or `*.test.tsx`?
- [ ] Every new public function or component has at least one test?
- [ ] Tests independent (no order dependencies)?
- [ ] Mocks reset in `beforeEach`?
- [ ] Web tests use `@testing-library/react`, not direct DOM manipulation?

### Cross-platform tests
- [ ] Platform helpers tested by mocking `process.platform`, not by running
      on each OS?

### Forbidden patterns
- [ ] No `sleep()` where `vi.useFakeTimers()` would work?
- [ ] No tests that depend on real network, real ports without fallback
      to ephemeral, or real external services?

---

## 12. Cross-cutting

### TypeScript
- [ ] No new `any` types (`@typescript-eslint/no-explicit-any: error`)?
- [ ] `import type` for type-only imports?
- [ ] Unused vars prefixed `_` (`argsIgnorePattern: "^_"`)?

### Conventions
- [ ] Conventional commit prefix (`feat:`, `fix:`, `refactor:`, `docs:`,
      `test:`, `chore:`, `perf:`, `ci:`)?
- [ ] Changeset added if a published package's surface changed?

### Documentation
- [ ] CLAUDE.md updated if a convention changed?
- [ ] `docs/CROSS_PLATFORM.md` updated if a new platform helper was added?

---

## Reviewer self-check before submitting findings

- Did I read `CLAUDE.md` for project conventions before flagging style issues?
- Did I read `docs/CROSS_PLATFORM.md` for the helper inventory before
  flagging any platform-touching code?
- Are my BLOCKER findings actually blockers, or am I being conservative?
- Did I distinguish between "this is wrong" and "I'd write it differently"?
- Did I check whether the plan declared something out of scope before flagging it?
