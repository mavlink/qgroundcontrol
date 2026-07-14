"""Version-precedence contract for GStreamer build configuration."""

from resolve_gstreamer_config import resolve_version


def test_version_precedence_is_override_then_platform_then_fallback() -> None:
    for values, expected in (
        (("1.2.3", "9.9.9", "0.0.0"), "1.2.3"),
        (("", "9.9.9", "0.0.0"), "9.9.9"),
        (("", "", "0.0.0"), "0.0.0"),
    ):
        assert resolve_version(*values) == expected
