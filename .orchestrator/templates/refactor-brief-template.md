<task>
Execute slice {{N}} of plan {{plan_path}}: {{slice_description}}.

Files to modify:
- {{file_1}} — {{change_summary}}
- {{file_2}} — {{change_summary}}
</task>

<context>
Repo: this repository (sprig-ao monorepo, pnpm workspace).
Plan path: {{plan_path}}
Slice number: {{N}}
TDD-Guard is active for `packages/*/src/**` but only fires on Claude Code edits, not yours.

Constraints:
- This is a REFACTOR. No behavior change.
- All tests under `{{verification_command}}` must stay green. Test counts must match pre-slice baseline.
- Do not change exported interface shapes in `packages/core/src/types.ts` unless this slice IS the contract change.
- Do not introduce new dependencies (no edits to `package.json`, `pnpm-lock.yaml`).
- Do not modify `CLAUDE.md`, `AGENTS.md`, `DESIGN.md`, or any `docs/` file.
- Preserve cross-platform invariants: never inline `process.platform === "win32"` (use helpers from `@aoagents/ao-core`).
</context>

<structured_output_contract>
Return a single Markdown block:

## summary
<1-3 lines: what changed, where>

## files_changed
- <path> — <+lines/-lines> — <one-line reason>

## tests_run
- command: `{{verification_command}}`
- result: <PASS count|FAIL count> — <failing test names if any>

## followups
- <next-slice prerequisites or risks observed; empty if none>
</structured_output_contract>

<default_follow_through_policy>
- Use `git mv` for renames so history is preserved.
- After every change, run the verification command and confirm green before declaring done.
- If a test fails, fix the implementation (do not modify the test to silence it).
- If you discover the slice plan is wrong (e.g., a referenced symbol doesn't exist), STOP
  and report in the `followups` section. Do not freelance.
- Stay inside the file scope listed in <task>. Do not "while we're here" edit unrelated
  files.
</default_follow_through_policy>

<verification_loop>
1. Read the plan at {{plan_path}} (just the slice {{N}} section).
2. Read the source files listed in <task>.
3. Apply the changes.
4. Run `{{verification_command}}`. If failing, iterate up to 2 times.
5. Snapshot result and return per the output contract.
</verification_loop>
