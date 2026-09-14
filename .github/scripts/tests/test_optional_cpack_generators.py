"""Configure the optional generators without needing their platform packaging tools."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

import pytest
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path


@pytest.mark.parametrize(
    ("module", "generator"),
    [("Bundle", "Bundle"), ("ProductBuild", "productbuild"), ("IFW", "IFW")],
)
def test_optional_generator_configuration(tmp_path: Path, module: str, generator: str) -> None:
    source = tmp_path / "source"
    source.mkdir()
    for name in ("deploy", "resources", ".github"):
        (source / name).symlink_to(REPO_ROOT / name, target_is_directory=True)
    (source / "README.md").write_text("QGC test")
    (source / "payload").write_text("test")
    (source / "CMakeLists.txt").write_text(
        f'''cmake_minimum_required(VERSION 3.25)
project(QGroundControl VERSION 5.0.0 LANGUAGES NONE)
list(APPEND CMAKE_MODULE_PATH "{REPO_ROOT}/cmake/install/CPack")
set(QGC_ORG_NAME QGroundControl)
set(QGC_PACKAGE_NAME org.mavlink.qgroundcontrol)
set(ENV{{QT_ROOT_DIR}} "")
install(FILES payload DESTINATION . COMPONENT Runtime)
include(CreateCPack{module})
'''
    )
    build = tmp_path / "build"
    result = subprocess.run(
        ["cmake", "-S", str(source), "-B", str(build)],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stdout + result.stderr
    config = (build / "CPackConfig.cmake").read_text()
    assert f'set(CPACK_GENERATOR "{generator}")' in config
    assert 'set(CPACK_COMPONENTS_ALL "Runtime")' in config
    if module == "IFW":
        assert 'set(CPACK_IFW_COMPONENT_RUNTIME_NAME "QGroundControl")' in config
        assert "installscript.js" in config
        assert 'set(CPACK_IFW_ROOT "/")' not in config
    if module == "ProductBuild":
        assert 'set(CPACK_PACKAGING_INSTALL_PREFIX "/Applications")' in config
        assert 'set(CPACK_PRODUCTBUILD_IDENTIFIER "org.mavlink.qgroundcontrol")' in config
