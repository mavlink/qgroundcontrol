#!/usr/bin/env python3
"""Generate an SBOM from a CMake build directory's CPM package metadata.

Parses CMakeCache.txt for CPM_PACKAGES and per-package VERSION/SOURCE_DIR,
then reads git metadata (remote URL, commit hash) from each source directory.

Usage:
    generate_cpm_sbom.py --build-dir DIR [--format cyclonedx|spdx] [--output FILE]

Outputs CycloneDX 1.6 JSON by default or SPDX 2.2 JSON for GitHub dependency submission.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.cmake import read_cache_dict
from common.git import run_git


def git_info(source_dir: Path) -> tuple[str, str]:
    """Return (remote_url, commit_hash) from a git working directory."""
    url = ""
    commit = ""
    try:
        result = run_git("remote", "get-url", "origin", cwd=source_dir, timeout=5)
        if result.returncode == 0:
            url = result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    try:
        result = run_git("rev-parse", "HEAD", cwd=source_dir, timeout=5)
        if result.returncode == 0:
            commit = result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    return url, commit


def normalize_git_url(url: str) -> str:
    """Convert git URL to HTTPS browse URL for purl."""
    url = url.removesuffix(".git")
    if url.startswith("git@github.com:"):
        url = "https://github.com/" + url[len("git@github.com:") :]
    return url


def make_purl(name: str, version: str, url: str, commit: str) -> str:
    """Build a Package URL (purl) for a CPM dependency."""
    normalized = normalize_git_url(url)
    m = re.match(r"https://github\.com/([^/]+/[^/]+)", normalized)
    if m:
        repo_path = m.group(1).lower()
        ref = version if version and version != "0" else commit[:12]
        purl = f"pkg:github/{repo_path}@{ref}" if ref else f"pkg:github/{repo_path}"
        return purl
    m = re.match(r"https://gitlab\.com/([^/]+/[^/]+)", normalized)
    if m:
        repo_path = m.group(1).lower()
        ref = version if version and version != "0" else commit[:12]
        return f"pkg:gitlab/{repo_path}@{ref}" if ref else f"pkg:gitlab/{repo_path}"
    return (
        f"pkg:generic/{name.lower()}@{version}"
        if version and version != "0"
        else f"pkg:generic/{name.lower()}"
    )


def generate_sbom(build_dir: Path) -> dict:
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.exists():
        print(f"Error: {cache_path} not found", file=sys.stderr)
        sys.exit(1)

    cache = read_cache_dict(cache_path)

    packages_str = cache.get("CPM_PACKAGES", "")
    if not packages_str:
        print("Warning: No CPM_PACKAGES found in cache", file=sys.stderr)

    package_names = [p for p in packages_str.split(";") if p]

    components = []
    for name in package_names:
        version = cache.get(f"CPM_PACKAGE_{name}_VERSION", "")
        source_dir = cache.get(f"CPM_PACKAGE_{name}_SOURCE_DIR", "")

        url, commit = "", ""
        if source_dir:
            url, commit = git_info(Path(source_dir))

        display_version = (
            version if version and version != "0" else commit[:12] if commit else "unknown"
        )

        component: dict = {
            "type": "library",
            "name": name,
            "version": display_version,
        }

        purl = make_purl(name, version, url, commit)
        component["purl"] = purl
        component["bom-ref"] = purl

        external_refs = []
        if url:
            browse_url = normalize_git_url(url)
            external_refs.append({"type": "vcs", "url": browse_url})
        if external_refs:
            component["externalReferences"] = external_refs

        if commit:
            component["hashes"] = [{"alg": "SHA-1", "content": commit}]

        components.append(component)

    sbom = {
        "$schema": "http://cyclonedx.org/schema/bom-1.6.schema.json",
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "metadata": {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "component": {
                "type": "application",
                "name": "QGroundControl",
                "bom-ref": "pkg:github/mavlink/qgroundcontrol",
            },
            "tools": {
                "components": [
                    {
                        "type": "application",
                        "name": "generate_cpm_sbom",
                        "version": "1.0.0",
                    }
                ]
            },
        },
        "components": components,
    }

    return sbom


def generate_spdx(build_dir: Path) -> dict:
    """Return an SPDX dependency snapshot suitable for GitHub submission."""
    cyclonedx = generate_sbom(build_dir)
    root_id = "SPDXRef-Package-QGroundControl"
    packages: list[dict] = [
        {
            "SPDXID": root_id,
            "name": "QGroundControl",
            "downloadLocation": "https://github.com/mavlink/qgroundcontrol",
            "filesAnalyzed": False,
            "licenseConcluded": "NOASSERTION",
            "licenseDeclared": "NOASSERTION",
            "copyrightText": "NOASSERTION",
            "externalRefs": [
                {
                    "referenceCategory": "PACKAGE_MANAGER",
                    "referenceType": "purl",
                    "referenceLocator": "pkg:github/mavlink/qgroundcontrol",
                }
            ],
        }
    ]
    relationships = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": root_id,
        }
    ]

    for index, component in enumerate(cyclonedx["components"], start=1):
        safe_name = re.sub(r"[^A-Za-z0-9.-]", "-", component["name"])
        package_id = f"SPDXRef-Package-{safe_name}-{index}"
        external_references = component.get("externalReferences", [])
        package = {
            "SPDXID": package_id,
            "name": component["name"],
            "versionInfo": component["version"],
            "downloadLocation": (
                external_references[0]["url"] if external_references else "NOASSERTION"
            ),
            "filesAnalyzed": False,
            "licenseConcluded": "NOASSERTION",
            "licenseDeclared": "NOASSERTION",
            "copyrightText": "NOASSERTION",
            "externalRefs": [
                {
                    "referenceCategory": "PACKAGE_MANAGER",
                    "referenceType": "purl",
                    "referenceLocator": component["purl"],
                }
            ],
        }
        if hashes := component.get("hashes"):
            package["checksums"] = [
                {"algorithm": item["alg"].replace("-", ""), "checksumValue": item["content"]}
                for item in hashes
            ]
        packages.append(package)
        relationships.append(
            {
                "spdxElementId": root_id,
                "relationshipType": "DEPENDS_ON",
                "relatedSpdxElement": package_id,
            }
        )

    return {
        "spdxVersion": "SPDX-2.2",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": "QGroundControl CPM dependencies",
        "documentNamespace": (
            f"https://github.com/mavlink/qgroundcontrol/dependency-sbom/{uuid.uuid4()}"
        ),
        "creationInfo": {
            "created": datetime.now(timezone.utc).isoformat(),
            "creators": ["Tool: generate_cpm_sbom-1.0.0"],
        },
        "packages": packages,
        "relationships": relationships,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True, help="CMake build directory")
    parser.add_argument(
        "--format",
        choices=("cyclonedx", "spdx"),
        default="cyclonedx",
        help="SBOM format (default: cyclonedx)",
    )
    parser.add_argument("--output", "-o", type=Path, help="Output file (default: stdout)")
    args = parser.parse_args()

    sbom = generate_spdx(args.build_dir) if args.format == "spdx" else generate_sbom(args.build_dir)
    output = json.dumps(sbom, indent=2)

    if args.output:
        args.output.write_text(output, encoding="utf-8")
        count = len(sbom["packages"]) - 1 if args.format == "spdx" else len(sbom["components"])
        print(f"SBOM written to {args.output} ({count} components)")
    else:
        print(output)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
