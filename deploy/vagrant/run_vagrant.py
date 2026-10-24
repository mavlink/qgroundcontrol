#!/usr/bin/env python3
"""Build QGC in a disposable Vagrant (libvirt) VM and copy its AppImage to OUTPUT_DIR.

``install`` first prepares an Ubuntu host: Vagrant from HashiCorp's apt repository, libvirt/KVM,
UEFI firmware, and the vagrant-libvirt plugin. ``build`` (the default) uses the Vagrantfile's
headless CI mode (``QGC_CI``, default ``1``; size via ``QGC_VM_CPUS``/``QGC_VM_MEM_MB``), stops
``vagrant up`` after ``QGC_VM_UP_TIMEOUT`` seconds (default 150 minutes), and destroys the VM
whenever it attempted to start one. It refuses to run, or destroy, a machine that already exists in
this project.
"""

from __future__ import annotations

import argparse
import contextlib
import getpass
import os
import platform
import shlex
import signal
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import NoReturn

VAGRANT_DIR = Path(__file__).resolve().parent
APT_INSTALL = VAGRANT_DIR.parents[1] / "tools/ci/runner/apt_install_retry.py"
PROVIDER = "libvirt"
HASHICORP_KEY_URL = "https://apt.releases.hashicorp.com/gpg"
HASHICORP_KEYRING = "/usr/share/keyrings/hashicorp-archive-keyring.gpg"
HASHICORP_SOURCES = "/etc/apt/sources.list.d/hashicorp.list"
HOST_PACKAGES = (
    "vagrant",
    "qemu-kvm",
    "qemu-utils",
    "libvirt-daemon-system",
    "libvirt-dev",
    "ovmf",
    "ebtables",
    "dnsmasq-base",
    "build-essential",
    "ruby-dev",
)
# Written by deploy/multipass/build_in_vm.py, which provision.py runs as the vagrant user.
GUEST_ARTIFACT_RECORD = "/home/vagrant/qgc-appimage-path"
KEY_TIMEOUT = 180
COMMAND_TIMEOUT = 300
PLUGIN_TIMEOUT = 1200
# Leaves the CI job's 180 minutes room for host setup and VM teardown.
UP_TIMEOUT = 150 * 60
COPY_TIMEOUT = 900
DESTROY_TIMEOUT = 600
KILL_GRACE = 30


def install() -> None:
    # Ubuntu 24.04 (noble) dropped the vagrant package; HashiCorp's repository publishes it.
    key = subprocess.run(
        [
            "curl",
            "--fail",
            "--show-error",
            "--silent",
            "--location",
            "--retry",
            "5",
            "--retry-delay",
            "2",
            "--retry-max-time",
            "120",
            "--connect-timeout",
            "10",
            "--max-time",
            "30",
            HASHICORP_KEY_URL,
        ],
        stdout=subprocess.PIPE,
        text=True,
        check=True,
        timeout=KEY_TIMEOUT,
    ).stdout
    subprocess.run(
        ["sudo", "gpg", "--batch", "--yes", "--dearmor", "-o", HASHICORP_KEYRING],
        input=key,
        text=True,
        check=True,
        timeout=COMMAND_TIMEOUT,
    )
    release = platform.freedesktop_os_release()
    codename = release.get("UBUNTU_CODENAME") or release.get("VERSION_CODENAME")
    if not codename:
        raise ValueError("os-release does not name an Ubuntu codename")
    subprocess.run(
        ["sudo", "tee", HASHICORP_SOURCES],
        input=(
            f"deb [signed-by={HASHICORP_KEYRING}] https://apt.releases.hashicorp.com "
            f"{codename} main\n"
        ),
        text=True,
        check=True,
        timeout=COMMAND_TIMEOUT,
    )
    # apt_install_retry.py bounds every apt attempt itself (under sudo, where it can kill apt).
    subprocess.run(
        [
            sys.executable,
            str(APT_INSTALL),
            "--update",
            "--no-install-recommends",
            "--label",
            "Vagrant and libvirt",
            *HOST_PACKAGES,
        ],
        check=True,
    )
    subprocess.run(
        ["sudo", "systemctl", "enable", "--now", "libvirtd"], check=True, timeout=COMMAND_TIMEOUT
    )
    user = getpass.getuser()
    subprocess.run(
        ["sudo", "usermod", "-aG", "libvirt,kvm", user], check=True, timeout=COMMAND_TIMEOUT
    )
    subprocess.run(
        ["sudo", "-u", user, "vagrant", "plugin", "install", "vagrant-libvirt"],
        check=True,
        timeout=PLUGIN_TIMEOUT,
    )


def _stop(process: subprocess.Popen[str]) -> None:
    with contextlib.suppress(ProcessLookupError, PermissionError):
        os.killpg(process.pid, signal.SIGTERM)
    try:
        process.wait(KILL_GRACE)
    except subprocess.TimeoutExpired:
        with contextlib.suppress(ProcessLookupError, PermissionError):
            os.killpg(process.pid, signal.SIGKILL)
        process.wait()


def vagrant(*arguments: str, timeout: float, capture: bool = False) -> str:
    # `sg libvirt` picks up the group membership `install` added without a re-login. The new
    # session lets a timeout or interrupt stop sg's shell and Vagrant's Ruby process together.
    command = ["sg", "libvirt", "-c", shlex.join(["vagrant", *arguments])]
    env = {"QGC_CI": "1", **os.environ, "VAGRANT_DEFAULT_PROVIDER": PROVIDER}
    with subprocess.Popen(
        command,
        cwd=VAGRANT_DIR,
        env=env,
        stdout=subprocess.PIPE if capture else None,
        text=True,
        start_new_session=True,
    ) as process:
        try:
            output, _ = process.communicate(timeout=timeout)
        except BaseException:
            _stop(process)
            raise
    if process.returncode:
        raise subprocess.CalledProcessError(process.returncode, command, output)
    return output or ""


def machine_state() -> str:
    output = vagrant("status", "--machine-readable", timeout=COMMAND_TIMEOUT, capture=True)
    for line in output.splitlines():
        fields = line.split(",", 3)
        if len(fields) == 4 and fields[2] == "state":
            return fields[3]
    raise ValueError("Vagrant did not report the machine state")


def copy_artifact(output: Path) -> Path:
    # The build ran in the guest's home, so copy the AppImage out over Vagrant's SSH settings.
    with tempfile.TemporaryDirectory(prefix="qgc-vagrant-") as staging:
        config = Path(staging) / "ssh-config"
        config.write_text(vagrant("ssh-config", timeout=COMMAND_TIMEOUT, capture=True))
        ssh = ["-F", str(config)]
        artifact = subprocess.run(
            ["ssh", *ssh, "default", "cat", GUEST_ARTIFACT_RECORD],
            stdout=subprocess.PIPE,
            text=True,
            check=True,
            timeout=COMMAND_TIMEOUT,
        ).stdout.strip()
        if not artifact.startswith("/") or "\n" in artifact or not artifact.endswith(".AppImage"):
            raise ValueError(f"Invalid VM artifact path: {artifact!r}")
        subprocess.run(
            ["scp", *ssh, f"default:{artifact}", f"{output}/"], check=True, timeout=COPY_TIMEOUT
        )
    return output / Path(artifact).name


def destroy() -> bool:
    try:
        vagrant("destroy", "-f", timeout=DESTROY_TIMEOUT)
        return True
    except (OSError, subprocess.SubprocessError) as error:
        print(f"VM cleanup failed: {error}", file=sys.stderr)
        return False


def build(output: Path, up_timeout: float) -> int:
    started = False
    status = 1
    try:
        output.mkdir(parents=True, exist_ok=True)
        # --ignore-provider: the Vagrantfile keeps a docker override (no image) for desktop use.
        vagrant("validate", "--ignore-provider", timeout=COMMAND_TIMEOUT)
        if (state := machine_state()) != "not_created":
            raise ValueError(f"Vagrant machine already exists ({state}); destroy it first")
        started = True
        vagrant("up", f"--provider={PROVIDER}", timeout=up_timeout)
        print(f"Output: {copy_artifact(output)}")
        status = 0
    except subprocess.CalledProcessError as error:
        print(f"VM build failed: {error}", file=sys.stderr)
        status = error.returncode if error.returncode > 0 else 1
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        print(f"VM build failed: {error}", file=sys.stderr)
    finally:
        if started and not destroy():
            status = status or 1
    return status


def _exit_on_sigterm(signum: int, _frame: object) -> NoReturn:
    raise SystemExit(128 + signum)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", nargs="?", choices=("install", "build"), default="build")
    args = parser.parse_args(argv)
    # Unwind on SIGTERM as on Ctrl-C, so build()'s finally still destroys the VM.
    signal.signal(signal.SIGTERM, _exit_on_sigterm)
    if args.command == "install":
        try:
            install()
            return 0
        except (OSError, ValueError, subprocess.SubprocessError) as error:
            print(f"Vagrant host setup failed: {error}", file=sys.stderr)
            return 1
    output = Path(os.environ.get("OUTPUT_DIR", Path.cwd())).resolve()
    return build(output, float(os.environ.get("QGC_VM_UP_TIMEOUT", UP_TIMEOUT)))


if __name__ == "__main__":
    raise SystemExit(main())
