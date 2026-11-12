"""qt-install saves a fresh Qt install only from runs that may write the cache."""

from __future__ import annotations

from ..._helpers import evaluate_expression, get_step, get_steps, load_action


def test_only_runs_that_may_write_the_cache_save_a_fresh_install() -> None:
    allowed = get_step(get_steps(load_action("qt-install")), "plan")["env"]["SAVE_ALLOWED"]

    def may_save(event: str, head_repo: str = "mavlink/qgroundcontrol") -> bool:
        pull_request = {"head": {"repo": {"full_name": head_repo}}}
        context = {
            "github": {
                "event_name": event,
                "repository": "mavlink/qgroundcontrol",
                "event": {"pull_request": pull_request} if event == "pull_request" else {},
            }
        }
        return bool(evaluate_expression(allowed, context))

    assert may_save("push") and may_save("schedule") and may_save("pull_request")
    # A fork's token cannot write the cache.
    assert not may_save("pull_request", head_repo="someone/qgroundcontrol")
    # Copilot sessions cannot write the cache; a save only logs "cache write denied".
    assert not may_save("dynamic")
