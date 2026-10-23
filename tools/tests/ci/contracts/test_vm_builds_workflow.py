"""Static contracts for the VM Builds workflow; VM lifecycle behavior is unit-tested elsewhere."""

from __future__ import annotations

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow

WORKFLOW = REPO_ROOT / ".github" / "workflows" / "vm-builds.yml"
ENABLE_KVM = "python3 tools/ci/runner/enable_kvm.py"
RUN_VAGRANT = "python3 deploy/vagrant/run_vagrant.py"


def test_pull_requests_skip_only_the_push_only_runs_on_config() -> None:
    on = load_workflow("vm-builds.yml")["on"]
    paths = on["push"]["paths"]
    # RunsOn reads runs-on.yml from the default branch, so PR runs would not exercise a change.
    assert on["pull_request"]["paths"] == [path for path in paths if path != ".github/runs-on.yml"]
    assert ".github/runs-on.yml" in paths
    # Everything the VM build runs before CMake (deploy/multipass/build_in_vm.py and
    # deploy/vagrant/provision.py), plus the CI-side helpers.
    for path in (
        "tools/ci/runner/apt_install_retry.py",
        "tools/ci/runner/enable_kvm.py",
        "tools/ci/runner/install_multipass.py",
        "tools/setup/install_uv.py",
        "tools/setup/install_python.py",
        "tools/setup/install_qt.py",
        "tools/setup/qt_installer/**",
        "tools/setup/install_dependencies/**",
        "deploy/multipass/**",
        "deploy/vagrant/**",
        ".github/build-config.json",
    ):
        assert path in paths


def test_both_vm_jobs_use_the_shared_kvm_helper() -> None:
    jobs = load_workflow("vm-builds.yml")["jobs"]
    for job in jobs.values():
        assert get_step(job["steps"], "Enable KVM")["run"] == ENABLE_KVM
    assert "kvm4all" not in WORKFLOW.read_text(encoding="utf-8")


def test_android_emulator_uses_the_shared_kvm_helper() -> None:
    action = REPO_ROOT / ".github" / "actions" / "android-emulator-test" / "action.yml"
    steps = get_steps(load_action("android-emulator-test"))
    assert get_step(steps, "Enable KVM")["run"] == ENABLE_KVM
    assert "kvm4all" not in action.read_text(encoding="utf-8")


def test_vagrant_job_delegates_lifecycle_to_run_vagrant() -> None:
    job = load_workflow("vm-builds.yml")["jobs"]["vagrant-build"]
    assert "continue-on-error" not in job
    install = get_step(job["steps"], "Install Vagrant + libvirt")
    build = get_step(job["steps"], "Build QGC in Vagrant VM")
    upload = get_step(job["steps"], "Upload AppImage")
    assert install["run"] == f"{RUN_VAGRANT} install"
    assert build["run"] == f"{RUN_VAGRANT} build"
    assert build["env"]["OUTPUT_DIR"] == "${{ github.workspace }}/artifacts"
    assert job["steps"].index(install) < job["steps"].index(build) < job["steps"].index(upload)
    assert upload["with"] == {
        "name": "qgc-vagrant-appimage",
        "path": "artifacts/*.AppImage",
        "if-no-files-found": "error",
    }
    # run_vagrant.py destroys the VM itself; a separate step would race or double-destroy it.
    for step in job["steps"]:
        run = step.get("run", "")
        assert "vagrant destroy" not in run
        assert "/tmp/" not in run


def test_only_the_multipass_job_opens_bridge_forwarding() -> None:
    # Docker's FORWARD DROP policy blocks the Multipass bridge NAT; libvirt adds its own rules.
    jobs = load_workflow("vm-builds.yml")["jobs"]
    install = get_step(
        jobs["multipass-build"]["steps"], "Install Multipass and allow VM bridge networking"
    )
    assert (
        install["run"] == "python3 tools/ci/runner/install_multipass.py --allow-bridge-forwarding"
    )
    for job in jobs.values():
        for step in job["steps"]:
            assert "iptables" not in step.get("run", "")
            assert "sysctl" not in step.get("run", "")
