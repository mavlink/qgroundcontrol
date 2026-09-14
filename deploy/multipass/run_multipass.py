#!/usr/bin/env python3
"""Build in a disposable Multipass VM and copy its AppImage to OUTPUT_DIR."""

from __future__ import annotations

import os
import subprocess
import sys
import tempfile
from pathlib import Path


def main() -> int:
    name = os.environ.get("MP_NAME", "qgc")
    output = Path(os.environ.get("OUTPUT_DIR", Path.cwd())).resolve()
    source = os.environ.get("QGC_SOURCE_DIR", "")
    created = False
    try:
        if source and not Path(source).is_dir():
            raise ValueError(f"Source directory does not exist: {source}")
        if (
            subprocess.run(
                ["multipass", "info", name],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False,
            ).returncode
            == 0
        ):
            raise ValueError(f"Multipass instance already exists: {name}")
        output.mkdir(parents=True, exist_ok=True)
        image = [os.environ["MP_IMAGE"]] if os.environ.get("MP_IMAGE") else []
        subprocess.run(
            [
                "multipass",
                "launch",
                *image,
                "--name",
                name,
                "--cpus",
                os.environ.get("MP_CPUS", "4"),
                "--memory",
                os.environ.get("MP_MEM", "8G"),
                "--disk",
                os.environ.get("MP_DISK", "25G"),
            ],
            check=True,
        )
        created = True
        guest_repo = "qgroundcontrol"
        if source:
            # Multipass's snap can read the home interface, but not the host's /tmp.
            with tempfile.TemporaryDirectory(prefix="qgc-src-", dir=Path.home()) as staging:
                archive = Path(staging) / "source.tar.gz"
                subprocess.run(["tar", "-C", source, "-czf", str(archive), "."], check=True)
                subprocess.run(
                    ["multipass", "transfer", str(archive), f"{name}:/tmp/qgc-src.tar.gz"],
                    check=True,
                )
            subprocess.run(["multipass", "exec", name, "--", "mkdir", "-p", guest_repo], check=True)
            subprocess.run(
                [
                    "multipass",
                    "exec",
                    name,
                    "--",
                    "tar",
                    "-C",
                    guest_repo,
                    "-xzf",
                    "/tmp/qgc-src.tar.gz",
                ],
                check=True,
            )
        else:
            subprocess.run(
                [
                    "multipass",
                    "exec",
                    name,
                    "--",
                    "git",
                    "clone",
                    "https://github.com/mavlink/qgroundcontrol.git",
                    guest_repo,
                    "--recurse-submodules",
                ],
                check=True,
            )
        subprocess.run(
            [
                "multipass",
                "exec",
                name,
                "--",
                "python3",
                f"{guest_repo}/deploy/multipass/build_in_vm.py",
            ],
            check=True,
        )
        artifact = subprocess.run(
            ["multipass", "exec", name, "--", "cat", "qgc-appimage-path"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
        if not artifact.startswith("/") or "\n" in artifact or not artifact.endswith(".AppImage"):
            raise ValueError(f"Invalid VM artifact path: {artifact!r}")
        subprocess.run(["multipass", "transfer", f"{name}:{artifact}", f"{output}/"], check=True)
        print(f"Output: {output / Path(artifact).name}")
        return 0
    except subprocess.CalledProcessError as error:
        print(f"VM build failed: {error}", file=sys.stderr)
        return error.returncode
    except (OSError, ValueError) as error:
        print(f"VM build failed: {error}", file=sys.stderr)
        return 1
    finally:
        if created:
            subprocess.run(["multipass", "delete", "--purge", name], check=False)


if __name__ == "__main__":
    raise SystemExit(main())
