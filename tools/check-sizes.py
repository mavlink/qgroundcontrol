#!/usr/bin/env python3
"""
Check artifact sizes against thresholds.

Usage:
    ./tools/check-sizes.py build/                    # Check local build artifacts
    ./tools/check-sizes.py artifacts/ --json         # Output JSON (for CI)
    ./tools/check-sizes.py artifacts/ --markdown     # Output markdown table
    ./tools/check-sizes.py --list-thresholds         # Show configured thresholds

Exit codes:
    0 - All artifacts within thresholds
    1 - One or more artifacts exceed thresholds
    2 - No artifacts found
"""

import argparse
import json
import sys
from pathlib import Path

# Size thresholds in MB - edit these values as needed
THRESHOLDS_MB = {
    ".AppImage": 200,
    ".dmg": 150,
    ".exe": 200,
    ".apk": 150,
    ".ipa": 150,
}

# File extensions to scan for
ARTIFACT_EXTENSIONS = {".AppImage", ".dmg", ".exe", ".apk", ".ipa", ".deb", ".rpm", ".zip", ".tar.gz"}


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
        size_mb = size_bytes / (1024 * 1024)
        threshold_mb = THRESHOLDS_MB.get(ext)

        artifacts.append({
            "name": path.name,
            "path": str(path),
            "extension": ext,
            "size_bytes": size_bytes,
            "size_mb": round(size_mb, 2),
            "size_human": format_size(size_bytes),
            "threshold_mb": threshold_mb,
            "exceeds_threshold": threshold_mb is not None and size_mb > threshold_mb,
        })

    return sorted(artifacts, key=lambda x: x["name"])


def output_json(artifacts: list[dict], warnings: list[str]) -> str:
    """Generate JSON output."""
    return json.dumps({
        "artifacts": artifacts,
        "warnings": warnings,
        "total_count": len(artifacts),
        "warning_count": len(warnings),
    }, indent=2)


def output_markdown(artifacts: list[dict], warnings: list[str]) -> str:
    """Generate markdown table output."""
    lines = [
        "## 📦 Artifact Sizes",
        "",
        "| Artifact | Size | Threshold | Status |",
        "|----------|------|-----------|--------|",
    ]

    for a in artifacts:
        threshold = f"{a['threshold_mb']} MB" if a["threshold_mb"] else "-"
        status = "⚠️ EXCEEDS" if a["exceeds_threshold"] else "✅"
        lines.append(f"| {a['name']} | {a['size_human']} | {threshold} | {status} |")

    if warnings:
        lines.extend(["", "### ⚠️ Warnings", ""])
        for w in warnings:
            lines.append(f"- {w}")

    return "\n".join(lines)


def output_text(artifacts: list[dict], warnings: list[str]) -> str:
    """Generate plain text output."""
    lines = ["Artifact Sizes:", ""]

    for a in artifacts:
        status = " ⚠️  EXCEEDS THRESHOLD" if a["exceeds_threshold"] else ""
        threshold_info = f" (threshold: {a['threshold_mb']} MB)" if a["threshold_mb"] else ""
        lines.append(f"  {a['name']}: {a['size_human']}{threshold_info}{status}")

    if warnings:
        lines.extend(["", "Warnings:"])
        for w in warnings:
            lines.append(f"  - {w}")

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(
        description="Check artifact sizes against thresholds",
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
        "--output", "-o",
        type=Path,
        help="Write output to file",
    )
    parser.add_argument(
        "--summary",
        type=Path,
        help="Append markdown summary to file (for GITHUB_STEP_SUMMARY)",
    )
    parser.add_argument(
        "--list-thresholds",
        action="store_true",
        help="List configured thresholds and exit",
    )
    parser.add_argument(
        "--no-fail",
        action="store_true",
        help="Don't exit with error code on threshold violations",
    )

    args = parser.parse_args()

    if args.list_thresholds:
        print("Configured thresholds:")
        for ext, mb in sorted(THRESHOLDS_MB.items()):
            print(f"  {ext}: {mb} MB")
        return 0

    # Find artifacts
    artifacts = find_artifacts(args.directory)

    if not artifacts:
        print(f"No artifacts found in {args.directory}", file=sys.stderr)
        return 2

    # Check for threshold violations
    warnings = []
    for a in artifacts:
        if a["exceeds_threshold"]:
            warnings.append(
                f"{a['name']} ({a['size_human']}) exceeds {a['threshold_mb']} MB threshold"
            )

    # Generate output
    if args.json:
        output = output_json(artifacts, warnings)
    elif args.markdown:
        output = output_markdown(artifacts, warnings)
    else:
        output = output_text(artifacts, warnings)

    # Write output
    if args.output:
        args.output.write_text(output)
        print(f"Output written to {args.output}")
    else:
        print(output)

    # Append to summary file (for GitHub Actions)
    if args.summary:
        with open(args.summary, "a") as f:
            f.write(output_markdown(artifacts, warnings))
            f.write("\n")

    # Exit code
    if warnings and not args.no_fail:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
