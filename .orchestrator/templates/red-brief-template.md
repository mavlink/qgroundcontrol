<task>
RED phase of slice {{N}}: write a failing test for {{behavior_description}}.

Test file: {{test_file_path}}
Module under test: {{module_path}} (does not yet exist or does not yet implement this behavior)

The test must fail at runtime (assertion, not collection/import error in a way that
prevents the rest of the suite from running). Match the test file structure used by the
target package — Vitest with `describe`/`it`/`expect`, mocks via `vi.mock`, React tests
via `@testing-library/react`.
</task>

<context>
Repo: this repository (sprig-ao monorepo).
Plan path: {{plan_path}}
Slice number: {{N}} (RED phase 1 of 2)
The next dispatch (GREEN) will implement the under-test code. Do not write that yet.

Constraints:
- DO NOT touch {{module_path}} in this dispatch. Only the test file.
- Test must fail when `{{verification_command}}` runs.
- Match existing test style in the target package's `__tests__/` folder.
- No `it.skip`, no `describe.skip`. The test must run and fail.
</context>

<structured_output_contract>
## summary
<1-2 lines>

## files_changed
- <test_file_path> — +<lines>

## tests_run
- command: `{{verification_command}}`
- result: 1 failed (<test name>) — <one-line failure reason>

## followups
- expected impl scope for GREEN phase: <hint>
</structured_output_contract>

<default_follow_through_policy>
- Test failure must come from the assertion or a clearly-scoped import error inside the
  test body, not from a parser/collection error that breaks the whole suite.
- Match existing test class naming and Vitest patterns in the target package.
</default_follow_through_policy>

<verification_loop>
1. Read existing tests in the target package's `__tests__/` folder to match style.
2. Write the failing test.
3. Run `{{verification_command}}` — confirm exactly one new failure with the right reason.
4. Return per output contract.
</verification_loop>
