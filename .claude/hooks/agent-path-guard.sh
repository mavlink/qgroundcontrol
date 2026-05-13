#!/usr/bin/env bash
# Per-subagent path-scoped write guard for sprig-ao.
#
# Runs as a PreToolUse hook on Edit|Write|MultiEdit, chained after
# tdd-guard-packages.sh. Blocks writes to production paths when the active
# subagent is a non-coder role.
#
# Hook input (stdin, JSON):
#   {
#     "tool_name": "...",
#     "tool_input": { "file_path": "..." },
#     ...  (Claude Code may include "agent_name", "subagent_name", or similar)
#   }
#
# Per-subagent write allowlist:
#   architect    -> .orchestrator/**, docs/**, CLAUDE.md, AGENTS.md,
#                   .gitignore, repo-root *.md
#   synthesizer  -> .orchestrator/review-synthesis.md only
#   reviewer     -> no writes anywhere
#   debug        -> no writes anywhere
#   codex-handoff-> no writes anywhere
#   (any other / no subagent) -> pass through (top-level session unrestricted
#                                  aside from tdd-guard-packages.sh)
#
# Blocking is signalled by writing a refusal to stderr and exiting non-zero
# (Claude Code surfaces the message back to the model).

set -euo pipefail

export PATH="$HOME/.npm-global/bin:$PATH"

input=$(cat)

extract() {
  local expr="$1" default="${2:-}"
  if command -v jq >/dev/null 2>&1; then
    printf '%s' "$input" | jq -r "$expr // \"\"" 2>/dev/null || printf '%s' "$default"
  else
    printf '%s' "$input" | python3 -c "
import json, sys
try:
    data = json.load(sys.stdin)
except Exception:
    sys.exit(0)
expr = '''$expr'''
parts = [p.strip() for p in expr.split('//')]
for p in parts:
    cur = data
    ok = True
    for key in p.lstrip('.').split('.'):
        if isinstance(cur, dict) and key in cur:
            cur = cur[key]
        else:
            ok = False
            break
    if ok and cur:
        print(cur if isinstance(cur, str) else json.dumps(cur))
        sys.exit(0)
print('$default')
" 2>/dev/null || printf '%s' "$default"
  fi
}

target=$(extract '.tool_input.file_path')
subagent=$(extract '.subagent_name // .agent_name // .subagent // .agent // .invocation.subagent_name')

target="${target%$'\n'}"
subagent="${subagent%$'\n'}"

if [[ -z "$subagent" ]]; then
  exit 0
fi

if [[ -z "$target" ]]; then
  exit 0
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

case "$target" in
  /*)
    if [[ "$target" == "$repo_root"/* ]]; then
      rel="${target#$repo_root/}"
    else
      rel="$target"
    fi
    ;;
  *)
    rel="$target"
    ;;
esac

block() {
  local reason="$1"
  cat >&2 <<EOF
agent-path-guard: BLOCKED write by subagent "$subagent" to "$rel".
Reason: $reason
Allowed scope for this subagent is documented in
.claude/agents/$subagent.md.
EOF
  exit 2
}

allow() {
  exit 0
}

is_repo_root_md() {
  [[ "$rel" == *.md && "$rel" != */* ]]
}

case "$subagent" in
  architect)
    case "$rel" in
      .orchestrator/*) allow ;;
      docs/*)          allow ;;
      CLAUDE.md|AGENTS.md|.gitignore) allow ;;
      *)
        if is_repo_root_md; then allow; fi
        block "architect may write only to .orchestrator/**, docs/**, CLAUDE.md, AGENTS.md, .gitignore, and repo-root *.md"
        ;;
    esac
    ;;
  synthesizer)
    case "$rel" in
      .orchestrator/review-synthesis.md) allow ;;
      *) block "synthesizer may write only to .orchestrator/review-synthesis.md" ;;
    esac
    ;;
  reviewer|debug|codex-handoff)
    block "this subagent must not write files; return findings as a message instead"
    ;;
  *)
    exit 0
    ;;
esac
