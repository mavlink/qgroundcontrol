# Orchestration runs

Each subdirectory is a `/orchestrate` task instance, named by kebab-cased slug.

## Layout per task

```
<slug>/
├── plan.md                — slice plan (copy of .orchestrator/plans/NN-*.md or drafted)
├── state.md               — append-only state log
├── slice-1-brief.md       — refactor brief sent to Codex (or red/green if TDD)
├── slice-1-red-brief.md   — RED phase brief (TDD slices only)
├── slice-1-green-brief.md — GREEN phase brief (TDD slices only)
├── slice-1-result.md      — Codex stdout verbatim
├── slice-1-diff.patch     — git diff snapshot
├── slice-1-diff.stat      — git diff --stat
└── slice-N-...            — repeat per slice
```

## Resuming

Re-running `/orchestrate <slug>` reads `state.md` and continues from the first incomplete
slice. Completed slices are not re-run.

## Tracking

Unlike the gcs convention, sprig-ao **tracks** `.orchestrator/work/` in git so
that other developers can see in-flight runs without pulling local state. The
canonical record of execution is still the git history (one commit per slice);
`work/` is the audit trail of how each commit was produced.
