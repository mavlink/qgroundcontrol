"""Static contracts for the VM Builds workflow; VM lifecycle behavior is unit-tested elsewhere."""

from __future__ import annotations

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow

WORKFLOW = REPO_ROOT / ".github" / "workflows" / "vm-builds.yml"
ENABLE_KVM = "python3 tools/ci/runner/enable_kvm.py"
RUN_VAGRANT = "python3 deploy/vagrant/run_vagrant.py"


def test_push_and_pull_request_share_one_paths_list() -> None:
    text = WORKFLOW.read_text(encoding="utf-8")
    assert "paths: &vm-build-paths\n" in text
    assert "paths: *vm-build-paths\n" in text
    assert "<<:" not in text
    on = load_workflow("vm-builds.yml")["on"]
    paths = on["push"]["paths"]
    assert on["pull_request"]["paths"] == paths
    for path in (
        "tools/ci/runner/apt_install_retry.py",
        "tools/ci/runner/enable_kvm.py",
        "tools/ci/runner/install_multipass.py",
        "deploy/multipass/**",
        "deploy/vagrant/**",
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
