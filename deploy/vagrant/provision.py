#!/usr/bin/env python3
"""Provision a disposable Vagrant guest and build QGC as the vagrant user."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path


def append_once(path: Path, line: str) -> None:
    content = path.read_text() if path.exists() else ""
    if line not in content.splitlines():
        path.write_text(content.rstrip("\n") + "\n" + line + "\n")


def provision() -> None:
    repo = Path(os.environ.get("QGC_SOURCE_DIR", "/vagrant")).resolve()
    home = Path("/home/vagrant")
    environment = Path("/opt/qgc-venv")
    ci = bool(os.environ.get("QGC_CI"))
    env = os.environ | {"DEBIAN_FRONTEND": "noninteractive"}
    env["QGC_PYTHON_ENV"] = str(environment)
    env["UV_INSTALL_DIR"] = "/usr/local/bin"
    env["PATH"] = f"{environment / 'bin'}:/usr/local/bin:{os.environ['PATH']}"
    apt = [
        "apt-get",
        "-y",
        "-o",
        "Dpkg::Options::=--force-confdef",
        "-o",
        "Dpkg::Options::=--force-confold",
    ]
    subprocess.run(["apt-get", "update"], check=True, env=env)
    subprocess.run([*apt, "dist-upgrade"], check=True, env=env)
    packages = [
        "libudev-dev",
        "speech-dispatcher",
        "wget",
        "curl",
        "python3-venv",
        "python3-dev",
        "patchelf",
        "rsync",
    ]
    if not ci:
        packages += ["xubuntu-desktop", "qtcreator"]
    subprocess.run([*apt, "install", *packages], check=True, env=env)
    for script, arguments in (
        ("tools/setup/install_uv.py", []),
        ("tools/setup/install_python.py", ["build,qt", "--environment", str(environment)]),
        ("tools/setup/install_dependencies", ["--platform", "debian"]),
    ):
        subprocess.run([sys.executable, str(repo / script), *arguments], check=True, env=env)
    # Qt and CMake helpers may synchronize locked groups while running as the guest user.
    subprocess.run(["chown", "-R", "vagrant:vagrant", str(environment)], check=True)
    if not ci:
        subprocess.run(["systemctl", "set-default", "graphical.target"], check=True)
    user = [
        "runuser",
        "-u",
        "vagrant",
        "--",
        "env",
        f"HOME={home}",
        f"QGC_PYTHON_ENV={environment}",
        f"PATH={env['PATH']}",
        f"BUILD_DIR={home / 'shadow_build'}",
        f"QT_OUT={os.environ.get('QT_OUT', str(home / 'Qt'))}",
        f"JOBS={os.environ.get('JOBS', str(os.cpu_count() or 1))}",
        "CODESIGN=nocodesign",
    ]
    subprocess.run(
        [*user, "git", "config", "--global", "--replace-all", "safe.directory", str(repo)],
        check=True,
    )
    subprocess.run(
        [*user, "git", "-C", str(repo), "submodule", "update", "--init", "--recursive"], check=True
    )
    bashrc = home / ".bashrc"
    append_once(bashrc, "export QGC_PYTHON_ENV=/opt/qgc-venv; export PATH=/opt/qgc-venv/bin:$PATH")
    fuse = Path("/etc/fuse.conf")
    if fuse.exists():
        fuse.write_text(fuse.read_text().replace("#user_allow_other", "user_allow_other"))
    # Preserve the disposable guest's existing FUSE/AppArmor workaround.
    subprocess.run(["systemctl", "disable", "--now", "apparmor"], check=True)
    subprocess.run(["aa-teardown"], check=True)
    Path("/etc/security/limits.d/fileno.conf").write_text("* soft nofile 2048\n")
    subprocess.run(
        [
            *user,
            str(environment / "bin/python"),
            str(repo / "deploy/multipass/build_in_vm.py"),
            "--skip-dependencies",
        ],
        check=True,
    )
    if not ci:
        subprocess.run(
            [
                *user,
                "rsync",
                "--delete",
                "-aPH",
                f"{home / 'shadow_build'}/",
                f"{repo / 'shadow_build'}/",
            ],
            check=True,
        )


def main() -> int:
    if os.geteuid() != 0:
        print("Run provisioning as root inside the Vagrant guest", file=sys.stderr)
        return 1
    try:
        provision()
        return 0
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"Vagrant provisioning failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
