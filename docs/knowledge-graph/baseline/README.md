# Knowledge graph baseline — deferred

The Phase 0 kickoff asked for an `/understand-anything` knowledge graph committed here. It was deferred. See `docs/decisions/002-defer-knowledge-graph-baseline.md` for the reasoning.

When the graph is generated (post-Phase 1, scoped per-subsystem rather than whole-repo), artifacts land here:

- `knowledge-graph.json` — the merged graph.
- `<subsystem>-knowledge-graph.json` — per-subsystem graphs that were merged into the above.
- `meta.json` — last-analyzed commit hash.
- `fingerprints.json` — file structural fingerprints for incremental updates.
