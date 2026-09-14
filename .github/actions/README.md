# GitHub Actions

This directory contains the local composite actions shared by QGroundControl workflows. See
[`../ci-overview.md`](../ci-overview.md) for the complete workflow, action, and script layout.

## External Action Reference Policy

QGroundControl intentionally references external GitHub Actions by stable major-version tag, such as
`actions/checkout@v7`, instead of pinning each action to a full commit SHA. Dependabot monitors these
references and proposes action updates, while [the zizmor policy](../zizmor.yml) requires a tag or
other ref pin and rejects unpinned branch references.
CodeQL excludes `actions/unpinned-tag` to match this policy; other security queries remain enabled.

Automated findings whose only concern is that a released action tag is mutable or is not a full
commit SHA are accepted under this repository policy and do not require a code change. Reference
this section when dismissing or responding to those findings. This exception does not apply to
unversioned actions, branch references such as `@main`, unknown actions, or any warning that reports
an additional security problem.

`test-phase` runs the shared Unit/Integration sequence and preserves reports and
execution durations after failures. Callers must gate it on a successful build.
`run-unit-tests` also accepts `build-type` and `tests-regex` for portable subsets;
empty selections are errors. `test-report` uses the Actions job summary and does
not require `checks: write` (Codecov upload separately requires `id-token: write`).
`download-all-artifacts` supports failed-build diagnostics and strict frozen release
snapshots. PR reporting allows absent diagnostic artifacts when tests did not run, but
download failures still fail reporting. Release and baseline downloads remain strict about
missing artifacts. `build-action` is retained for consumers needing the combined build flow.

`cache` restores compiler/moc archives and immutable CPM/SDK snapshots.
`save-build-cache` checkpoints compiler/moc archives once per job after successful
compilation (or analysis autogen), honoring the policy exported by `cache`. Call it
only after verified success; Docker uses the compilation status in its performance report.

GitHub-backed compiler/moc snapshots use uncompressed tar artifacts with one-day retention,
so concurrent SDK and dependency cache writes cannot evict a just-completed build's objects.
Restore accepts the same workflow, source repository, branch, and event class; new branches
can also use a trusted default-branch snapshot. Names include configuration/compiler keys
and branch scope. Snapshot failures fall back to the dependency cache. RunsOn/S3 retains
its existing cache/save route. Artifacts use artifact storage rather than the dependency-cache
quota; one-day expiry bounds retention, and a cold build remains possible after expiry.

`cmake-build` writes per-invocation Ninja reports and uploads `ninja.log`, `report.json`, and
`report.md` even after compilation fails. Reports separate MOC/code generation and linking,
deduplicate commands with multiple outputs, and exclude earlier builds in the same directory.
Docker includes the same reports in its performance artifact. Non-Ninja generators skip them.
