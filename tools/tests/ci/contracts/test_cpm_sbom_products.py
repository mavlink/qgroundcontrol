"""The CPM dependency SBOM knows the NVD product of every package CMake fetches, or that NVD has none."""

from __future__ import annotations

import re

from ci.security.generate_cpm_sbom import NVD_PRODUCTS

from ..._helpers import REPO_ROOT

CPM_PACKAGE = re.compile(r"\bCPMAddPackage\(\s*NAME\s+([\w.+-]+)")


def test_every_cpm_package_has_an_nvd_product_entry() -> None:
    declared: set[str] = set()
    cmake_files = [REPO_ROOT / "CMakeLists.txt"]
    for directory in ("cmake", "src", "test"):
        cmake_files += (REPO_ROOT / directory).rglob("CMakeLists.txt")
        cmake_files += (REPO_ROOT / directory).rglob("*.cmake")
    for path in cmake_files:
        name = path.relative_to(REPO_ROOT).as_posix()
        # The CMake script tests fetch fixture repositories, and CPM.cmake is CPM itself.
        if name.startswith("cmake/tests/") or name == "cmake/modules/CPM.cmake":
            continue
        declared.update(CPM_PACKAGE.findall(path.read_text(encoding="utf-8")))

    # A new package needs its product, or None, and a removed one leaves no stale entry.
    assert declared == set(NVD_PRODUCTS) - {"GStreamer"}
