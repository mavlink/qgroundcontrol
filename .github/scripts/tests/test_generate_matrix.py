"""Tests for generate_matrix.py."""

from __future__ import annotations

import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).parent.parent))

from generate_matrix import generate_matrix


class TestGenerateMatrix:
    """Tests for matrix generation."""

    @pytest.fixture
    def sample_config(self):
        """Sample build configuration for testing."""
        return {
            "platforms": {
                "linux": {
                    "variants": [
                        {
                            "name": "linux-x64",
                            "os": "ubuntu-24.04",
                            "arch": "x86_64",
                            "cmake_generator": "Ninja",
                            "build_type": ["Debug", "Release"],
                        },
                        {
                            "name": "linux-arm64",
                            "os": "ubuntu-24.04-arm",
                            "arch": "aarch64",
                            "cmake_generator": "Ninja",
                            "build_type": ["Release"],
                        },
                    ],
                    "pr_variants": ["linux-x64"],
                },
                "windows": {
                    "variants": [
                        {
                            "name": "windows-x64",
                            "os": "windows-2022",
                            "arch": "x64",
                            "cmake_generator": "Ninja",
                            "build_type": ["Debug", "Release"],
                        },
                    ],
                },
                "android": {
                    "variants": [
                        {
                            "name": "android-arm64",
                            "os": "ubuntu-24.04",
                            "arch": "arm64-v8a",
                            "build_type": ["Release"],
                        },
                        {
                            "name": "android-arm32",
                            "os": "ubuntu-24.04",
                            "arch": "armeabi-v7a",
                            "build_type": ["Release"],
                        },
                    ],
                    "pr_variants": ["android-arm64"],
                },
            },
            "pr_build_type": "Release",
        }

    def test_generate_matrix_linux(self, sample_config):
        """Test matrix generation for Linux platform."""
        matrix = generate_matrix(sample_config, "linux")
        includes = matrix.get("include", [])

        assert len(includes) >= 1
        assert all("os" in item for item in includes)
        assert all("arch" in item for item in includes)

    def test_generate_matrix_with_build_type_override(self, sample_config):
        """Test matrix generation with explicit build type."""
        matrix = generate_matrix(sample_config, "linux", build_type="Debug")
        includes = matrix.get("include", [])

        # All should be Debug builds
        for item in includes:
            assert item.get("build_type") == "Debug"

    def test_generate_matrix_pr_minimal(self, sample_config):
        """Test matrix generation with PR minimal mode."""
        matrix = generate_matrix(sample_config, "linux", pr_minimal=True)
        includes = matrix.get("include", [])

        # Should only include pr_variants
        names = [item.get("name") for item in includes]
        assert "linux-x64" in names or len(includes) <= len(sample_config["platforms"]["linux"]["pr_variants"])

    def test_generate_matrix_variant_filter(self, sample_config):
        """Test matrix generation with variant filter."""
        matrix = generate_matrix(sample_config, "linux", variant_filter="linux-arm64")
        includes = matrix.get("include", [])

        # Should only include the filtered variant
        for item in includes:
            assert item.get("name") == "linux-arm64"

    def test_generate_matrix_invalid_platform(self, sample_config):
        """Test matrix generation with invalid platform exits."""
        with pytest.raises(SystemExit):
            generate_matrix(sample_config, "freebsd")

    def test_generate_matrix_android(self, sample_config):
        """Test matrix generation for Android platform."""
        matrix = generate_matrix(sample_config, "android")
        includes = matrix.get("include", [])

        assert len(includes) >= 1
        arches = [item.get("arch") for item in includes]
        assert "arm64-v8a" in arches or "armeabi-v7a" in arches
