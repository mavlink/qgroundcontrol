<task>
GREEN phase of slice {{N}}: implement {{module_path}} so that the failing test
{{failing_test_id}} passes.

The RED phase added the failing test in dispatch {{red_dispatch_id}}. Read the test, then
implement the minimum production code that makes it pass.
</task>

<context>
Repo: this repository (sprig-ao monorepo).
Plan path: {{plan_path}}
Slice number: {{N}} (GREEN phase 2 of 2)
RED phase result: {{red_summary}}

Constraints:
- Implement only what the failing test requires. No extra features.
- All prior tests in `{{verification_command}}` must remain green; the failing test must turn green.
- Do not modify the test file. If the test is wrong, STOP and report.
- Do not change exported interface shapes in `packages/core/src/types.ts` unless this slice IS the contract change.
- Preserve cross-platform invariants: never inline `process.platform === "win32"` (use helpers from `@aoagents/ao-core`).
- For agent plugins: preserve the `getActivityState` cascade (process check → JSONL actionable → native signal → JSONL fallback → null). The fallback step is mandatory.
</context>

<structured_output_contract>
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
- Minimal impl. Don't preempt the next slice's work.
- After impl, run the verification command and confirm green.
- If iteration is needed, max 3 attempts before halting and reporting.
</default_follow_through_policy>

<verification_loop>
1. Read the failing test at {{test_file_path}}.
2. Implement the under-test code at {{module_path}}.
3. Run `{{verification_command}}`. Confirm green.
4. Return.
</verification_loop>
