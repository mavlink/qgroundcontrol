#!/usr/bin/env python3
"""Install apt packages with bounded retries and a runner-owned download cache.

On RunsOn, ``--update`` first moves the Ubuntu archive to the in-region EC2 mirror.
"""

from __future__ import annotations

import argparse
import os
import re
import socket
import subprocess
import sys
import time
from pathlib import Path
from urllib.parse import urlsplit

MAX_ATTEMPTS = 3
UPDATE_TIMEOUT = 300
INSTALL_TIMEOUT = 1200
KILL_GRACE = 30

APT_DIR = Path("/etc/apt")
# Plain HTTP: not every region serves the ports mirror over HTTPS, and apt verifies
# the signed Release files either way, as on Canonical's own EC2 images.
EC2_MIRRORS = {
    "archive.ubuntu.com": "{region}.ec2.archive.ubuntu.com",
    "ports.ubuntu.com": "{region}.ec2.ports.ubuntu.com",
}
_REGION = re.compile(r"[a-z]{2}(?:-[a-z]+)+-[0-9]+")
_ONE_LINE_ENTRY = re.compile(
    r"^(?P<prefix>[ \t]*deb(?:-src)?[ \t]+(?:\[[^\]\n]*\][ \t]+)?)"
    r"(?P<uri>\S+)(?P<gap>[ \t]+)(?P<suite>\S+)",
    re.MULTILINE,
)
_DEB822_SUITES = re.compile(r"^Suites:(.*)$", re.MULTILINE | re.IGNORECASE)
_DEB822_URIS = re.compile(r"^(URIs:)(.*)$", re.MULTILINE | re.IGNORECASE)


def _positive_seconds(value: str) -> int:
    seconds = int(value)
    if seconds <= 0:
        raise argparse.ArgumentTypeError("timeout must be positive")
    return seconds


def _mirror_uri(uri: str, mirrors: dict[str, str]) -> str:
    parts = urlsplit(uri)
    mirror = mirrors.get(parts.netloc) if parts.scheme in ("http", "https") else None
    return parts._replace(scheme="http", netloc=mirror).geturl() if mirror else uri


def _is_security(suites: str) -> bool:
    # Like Canonical's EC2 images, security pockets stay on the primary archive host so
    # fixes never wait for a mirror sync; -updates on the mirror carries them too.
    return any(suite.endswith("-security") for suite in suites.split())


def rewrite_sources(text: str, mirrors: dict[str, str], *, deb822: bool) -> str:
    """Return apt sources with non-security archive entries moved to ``mirrors``."""

    def one_line_entry(match: re.Match[str]) -> str:
        if _is_security(match["suite"]):
            return match[0]
        uri = _mirror_uri(match["uri"], mirrors)
        return f"{match['prefix']}{uri}{match['gap']}{match['suite']}"

    def uris_field(match: re.Match[str]) -> str:
        return match[1] + re.sub(r"\S+", lambda uri: _mirror_uri(uri[0], mirrors), match[2])

    if not deb822:
        return _ONE_LINE_ENTRY.sub(one_line_entry, text)
    stanzas = re.split(r"(\n[ \t]*\n)", text)
    for index in range(0, len(stanzas), 2):
        suites = _DEB822_SUITES.search(stanzas[index])
        if not (suites and _is_security(suites[1])):
            stanzas[index] = _DEB822_URIS.sub(uris_field, stanzas[index])
    return "".join(stanzas)


def _resolves(host: str) -> bool:
    try:
        socket.getaddrinfo(host, 80)
    except OSError:
        return False
    return True


def _write_root_file(path: Path, text: str) -> None:
    subprocess.run(
        ["sudo", "tee", str(path)], input=text, text=True, stdout=subprocess.DEVNULL, check=True
    )


def restore_sources(originals: dict[Path, str]) -> None:
    for path, text in originals.items():
        try:
            _write_root_file(path, text)
        except (OSError, subprocess.CalledProcessError) as error:
            print(f"::warning::Could not restore {path}: {error}")


def use_ec2_mirror(apt_dir: Path) -> dict[Path, str]:
    """Move Ubuntu archive sources to RunsOn's in-region EC2 mirror; return replaced files.

    archive.ubuntu.com sometimes serves EC2 runners at a few hundred kB/s. A no-op off
    RunsOn, for other distributions, when already applied, and when the region has no
    mirror. Errors restore the original sources instead of failing the job.
    """
    region = os.environ.get("RUNS_ON_AWS_REGION", "")
    if not _REGION.fullmatch(region):
        return {}
    # Not every region has a ports mirror.
    mirrors = {host: mirror.format(region=region) for host, mirror in EC2_MIRRORS.items()}
    mirrors = {host: mirror for host, mirror in mirrors.items() if _resolves(mirror)}
    originals: dict[Path, str] = {}
    try:
        sources = apt_dir / "sources.list.d"
        paths = [apt_dir / "sources.list", *sorted(sources.glob("*.list"))]
        paths += sorted(sources.glob("*.sources"))
        for path in paths:
            if not path.is_file():
                continue
            text = path.read_text(encoding="utf-8")
            rewritten = rewrite_sources(text, mirrors, deb822=path.suffix == ".sources")
            if rewritten != text:
                originals[path] = text
                _write_root_file(path, rewritten)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"::warning::Keeping the original apt sources: {error}")
        restore_sources(originals)
        return {}
    if originals:
        print(f"Using the {region} EC2 apt mirror in {', '.join(map(str, originals))}")
    return originals


def _retry(
    options: list[str],
    operation: list[str],
    timeout: int,
    label: str,
    attempts: int = MAX_ATTEMPTS,
) -> bool:
    # timeout must run under sudo: an unprivileged Python timeout cannot kill
    # root-owned apt processes. Mirror timeouts also bound stalled connections.
    command = [
        "sudo",
        "timeout",
        "-k",
        str(KILL_GRACE),
        str(timeout),
        "apt-get",
        *options,
        *operation,
    ]
    for attempt in range(1, attempts + 1):
        if subprocess.run(command, check=False).returncode == 0:
            return True
        if attempt < attempts:
            print(f"::warning::{label} attempt {attempt} failed (stall/timeout or error); retrying")
            time.sleep(attempt * 15)
    return False


def _update(options: list[str]) -> bool:
    originals = use_ec2_mirror(APT_DIR)
    if originals:
        if _retry(options, ["update", "-qq"], UPDATE_TIMEOUT, "apt update (EC2 mirror)", 1):
            return True
        print("::warning::apt update from the EC2 mirror failed; restoring the original sources")
        restore_sources(originals)
    return _retry(options, ["update", "-qq"], UPDATE_TIMEOUT, "apt update")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--no-install-recommends", action="store_true")
    parser.add_argument("--autoclean", action="store_true")
    parser.add_argument("--install-timeout", type=_positive_seconds, default=INSTALL_TIMEOUT)
    parser.add_argument("--label", default="apt packages")
    parser.add_argument("packages", nargs="*")
    args = parser.parse_args(argv)
    if not args.packages and not args.update:
        parser.error("packages are required unless --update is given")

    cache_dir = Path.home() / ".cache/qgc-apt-archives"
    partial_dir = cache_dir / "partial"
    options = [
        "-o",
        "Acquire::Retries=3",
        "-o",
        "Acquire::http::Timeout=30",
        "-o",
        "Acquire::https::Timeout=30",
        "-o",
        "DPkg::Lock::Timeout=300",
        "-o",
        f"Dir::Cache::Archives={cache_dir}",
        "-o",
        "APT::Keep-Downloaded-Packages=true",
    ]
    try:
        cache_dir.mkdir(parents=True, exist_ok=True)
        try:
            subprocess.run(["sudo", "mkdir", "-p", str(partial_dir)], check=True)
            apt_user = subprocess.run(
                ["id", "_apt"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False
            )
            # Match apt's sandbox-owned partial directory to avoid root downloads.
            if apt_user.returncode == 0:
                subprocess.run(["sudo", "chown", "_apt:root", str(partial_dir)], check=True)
            subprocess.run(["sudo", "chmod", "700", str(partial_dir)], check=True)

            if args.update and not _update(options):
                print(f"::error::apt update failed after {MAX_ATTEMPTS} attempts", file=sys.stderr)
                return 1
            if not args.packages:
                return 0
            operation = ["install", "-y"]
            if args.no_install_recommends:
                operation.append("--no-install-recommends")
            if not _retry(
                options, [*operation, *args.packages], args.install_timeout, f"{args.label} install"
            ):
                print(
                    f"::error::{args.label} installation failed after {MAX_ATTEMPTS} attempts",
                    file=sys.stderr,
                )
                return 1
            return 0
        finally:
            try:
                if args.autoclean:
                    subprocess.run(["sudo", "apt-get", *options, "autoclean", "-qq"], check=False)
            finally:
                # Restore cache ownership even after failure so a retry can reuse downloads.
                subprocess.run(["sudo", "rm", "-rf", str(partial_dir)], check=True)
                subprocess.run(
                    ["sudo", "chown", "-R", f"{os.getuid()}:{os.getgid()}", str(cache_dir)],
                    check=True,
                )
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"::error::apt installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
