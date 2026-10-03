"""Collect changed-line review data from the existing compiler diagnostic stream."""

from __future__ import annotations

import json
import os
import re
from pathlib import Path
from typing import Any

MAX_FINDINGS = 50
MAX_MESSAGE = 2000
MAX_CONTEXT = 1000
DIAGNOSTIC = re.compile(
    r"^(?P<path>.+):(?P<line>\d+):(?P<column>\d+): "
    r"(?P<level>warning|error): (?P<message>.+) \[(?P<check>[\w.,=-]+)\]$"
)
ANSI = re.compile(r"\x1b\[[0-9;]*m")


class ReviewFindings:
    def __init__(self, root: Path, ranges: dict[Path, list[tuple[int, int]]]) -> None:
        self.root = root.resolve()
        self.ranges = ranges
        self.findings: dict[tuple[str, int, str, str], dict[str, Any]] = {}
        self.truncated = False

    def collect(self, output: str, directory: Path) -> None:
        lines = ANSI.sub("", output).splitlines()
        for index, text in enumerate(lines):
            match = DIAGNOSTIC.fullmatch(text)
            if not match:
                continue
            number = int(match["line"])
            candidates = [(root / match["path"]).resolve() for root in (directory, self.root)]
            path = next(
                (
                    candidate
                    for candidate in candidates
                    if candidate.is_relative_to(self.root)
                    and any(start <= number <= end for start, end in self.ranges.get(candidate, []))
                ),
                None,
            )
            if path is None:
                continue
            relative = path.relative_to(self.root).as_posix()
            key = (relative, number, match["check"], match["message"])
            if key in self.findings:
                continue
            if len(self.findings) >= MAX_FINDINGS:
                self.truncated = True
                continue
            context = []
            for following in lines[index + 1 : index + 5]:
                if not re.match(r"^\s*(?:\d+\s*\||\||[\^~])", following):
                    break
                context.append(following)
            message = match["message"]
            snippet = "\n".join(context)
            self.truncated |= (
                len(message) > MAX_MESSAGE
                or len(snippet) > MAX_CONTEXT
                or len(match["check"]) > 200
            )
            self.findings[key] = {
                "path": relative,
                "line": number,
                "check": match["check"][:200],
                "level": match["level"],
                "message": message[:MAX_MESSAGE],
                "context": snippet[:MAX_CONTEXT],
            }

    def write(self, destination: Path, tool: str, *, incomplete: bool) -> None:
        event = json.loads(Path(os.environ["GITHUB_EVENT_PATH"]).read_text(encoding="utf-8"))
        if os.environ["GITHUB_EVENT_NAME"] != "pull_request":
            raise ValueError("Review export requires a pull_request event")
        pr = event["pull_request"]
        report = {
            "manifest": {
                "schema": 1,
                "repository": os.environ["GITHUB_REPOSITORY"],
                "run_id": int(os.environ["GITHUB_RUN_ID"]),
                "run_attempt": int(os.environ["GITHUB_RUN_ATTEMPT"]),
                "pr_number": pr["number"],
                "head_sha": pr["head"]["sha"],
                "tool": tool,
                "incomplete": incomplete,
                "truncated": self.truncated,
            },
            "findings": sorted(
                self.findings.values(), key=lambda item: (item["path"], item["line"], item["check"])
            ),
        }
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_text(json.dumps(report, ensure_ascii=False) + "\n", encoding="utf-8")
