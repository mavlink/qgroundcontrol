#!/usr/bin/env python3
"""
Report artifact sizes.

Usage:
    ./check_sizes.py build/                    # Check local build artifacts
    ./check_sizes.py artifacts/ --json         # Output JSON (for CI)
    ./check_sizes.py artifacts/ --markdown     # Output markdown table

Exit codes:
    0 - Success
    2 - No artifacts found
"""

import argparse
import json
import sys
from pathlib import Path

# File extensions to scan for
ARTIFACT_EXTENSIONS = {
    ".AppImage",
    ".dmg",
    ".exe",
    ".apk",
    ".ipa",
    ".deb",
    ".rpm",
    ".zip",
    ".tar.gz",
}


def format_size(size_bytes: int) -> str:
    """Format bytes as human-readable string."""
    for unit in ["B", "KB", "MB", "GB"]:
        if size_bytes < 1024:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024
    return f"{size_bytes:.2f} TB"


def find_artifacts(directory: Path) -> list[dict]:
    """Recursively find artifacts in directory."""
    artifacts = []

    if not directory.exists():
        return artifacts

    for path in directory.rglob("*"):
        if not path.is_file():
            continue

        # Check extension (handle .tar.gz specially)
        ext = "".join(path.suffixes) if path.suffixes else path.suffix
        if ext not in ARTIFACT_EXTENSIONS:
            ext = path.suffix
            if ext not in ARTIFACT_EXTENSIONS:
                continue

        size_bytes = path.stat().st_size

        # Use parent directory name if it differs from filename (for disambiguation)
        # e.g., "QGroundControl-mac/QGroundControl.apk" -> "QGroundControl-mac.apk"
        parent_name = path.parent.name
        if parent_name and parent_name != directory.name and parent_name != path.stem:
            display_name = f"{parent_name}{ext}"
        else:
            display_name = path.name

        artifacts.append(
            {
                "name": display_name,
                "path": str(path),
                "extension": ext,
                "size_bytes": size_bytes,
                "size_mb": round(size_bytes / (1024 * 1024), 2),
                "size_human": format_size(size_bytes),
            }
        )

    return sorted(artifacts, key=lambda x: x["name"])


def output_json(artifacts: list[dict]) -> str:
    """Generate JSON output."""
    return json.dumps(
        {
            "artifacts": artifacts,
            "total_count": len(artifacts),
        },
        indent=2,
    )


def output_markdown(artifacts: list[dict]) -> str:
    """Generate markdown table output."""
    lines = [
        "## 📦 Artifact Sizes",
        "",
        "| Artifact | Size |",
        "|----------|------|",
    ]

    for a in artifacts:
        lines.append(f"| {a['name']} | {a['size_human']} |")

    return "\n".join(lines)


def output_text(artifacts: list[dict]) -> str:
    """Generate plain text output."""
    lines = ["Artifact Sizes:", ""]

    for a in artifacts:
        lines.append(f"  {a['name']}: {a['size_human']}")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Report artifact sizes",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument(
        "directory",
        nargs="?",
        type=Path,
        default=Path("build"),
        help="Directory to scan for artifacts (default: build)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Output as JSON",
    )
    parser.add_argument(
        "--markdown",
        action="store_true",
        help="Output as markdown table",
    )
    parser.add_argument(
        "--output",
        "-o",
        type=Path,
        help="Write output to file",
    )
    parser.add_argument(
        "--summary",
        type=Path,
        help="Append markdown summary to file (for GITHUB_STEP_SUMMARY)",
    )

    args = parser.parse_args()

    # Find artifacts
    artifacts = find_artifacts(args.directory)

    if not artifacts:
        print(f"No artifacts found in {args.directory}", file=sys.stderr)
        return 2

    # Generate output
    if args.json:
        output = output_json(artifacts)
    elif args.markdown:
        output = output_markdown(artifacts)
    else:
        output = output_text(artifacts)

    # Write output
    if args.output:
        args.output.write_text(output)
        print(f"Output written to {args.output}")
    else:
        print(output)

    # Append to summary file (for GitHub Actions)
    if args.summary:
        with open(args.summary, "a") as f:
            f.write(output_markdown(artifacts))
            f.write("\n")

    return 0


if __name__ == "__main__":
    sys.exit(main())
