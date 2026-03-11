from __future__ import annotations

from plan_docker_builds import plan_builds


def test_plan_builds_pull_request_filters_by_changes():
    plan = plan_builds("pull_request", linux_changed=True, android_changed=False)
    assert plan["has_jobs"] is True
    assert plan["matrix"]["include"] == [
        {
            "platform": "Linux",
            "dockerfile": "Dockerfile-build-ubuntu",
            "fuse": True,
            "artifact_pattern": "*.AppImage",
        }
    ]


def test_plan_builds_push_includes_all():
    plan = plan_builds("push", linux_changed=False, android_changed=False)
    assert len(plan["matrix"]["include"]) == 2
