"""Qt online-repository layout and package selection for Qt 6.8 and newer.

Each combination's metadata is ``online/qtsdkrepository/<host>/<target>/qt6_<tag>/qt6_<tag><suffix>/
Updates.xml``; an archive lives at ``<that dir>/<package>/<package version><archive>`` and extracts
where its package's Extract operation says. Android uses ``all_os`` for every host, the Windows x64
desktop repository splits per arch (6.11+), and iOS splits per SDK with base packages named
``qtforios.<sdk>`` (6.12+).
"""

from __future__ import annotations

import sys
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from xml.etree.ElementTree import Element

REPOSITORY = "online/qtsdkrepository"
_HOST_SEGMENTS = {
    "linux": "linux_x64",
    "linux_arm64": "linux_arm64",
    "mac": "mac_x64",
    "windows": "windows_x86",
    "windows_arm64": "windows_arm64",
}
_HOST_DESKTOP_ARCHES = {
    "linux": "linux_gcc_64",
    "linux_arm64": "linux_gcc_arm64",
    "mac": "clang_64",
    "windows": "win64_msvc2022_64",
    "windows_arm64": "win64_msvc2022_arm64",
}
_ARCH_DIR_PREFIXES = (("linux_", ""), ("win64_", ""))


def resolve_arch_dir(arch: str) -> str:
    """Map a Qt arch identifier to the on-disk directory name aqtinstall creates."""
    arch = arch.removesuffix("_cross_compiled")
    for prefix, replacement in _ARCH_DIR_PREFIXES:
        if arch.startswith(prefix):
            return replacement + arch[len(prefix) :]
    if arch == "clang_64":
        return "macos"
    return arch


def resolve_windows_host_arch(arch: str) -> str:
    """Return the native x64 Qt arch paired with a Windows ARM64 cross arch."""
    suffix = "_arm64_cross_compiled"
    if not arch.startswith("win64_msvc") or not arch.endswith(suffix):
        raise ValueError(f"Not a Windows ARM64 cross-compiled Qt architecture: {arch}")
    return f"{arch.removesuffix(suffix)}_64"


def effective_host(host: str) -> str:
    """Resolve the ``all_os`` host to the running platform, as aqt does."""
    if host != "all_os":
        return host
    if sys.platform.startswith("linux"):
        return "linux"
    return "mac" if sys.platform == "darwin" else "windows"


def is_cross_arch(target: str, arch: str) -> bool:
    """Whether the Qt runs its host tools from a separate desktop Qt."""
    return target in ("android", "ios") or arch.endswith("_cross_compiled")


def host_desktop_arch(host: str, arch: str) -> str:
    """Desktop Qt arch whose host tools a mobile or cross-compiled *arch* uses.

    aqt's choice, except that Android on Windows pairs with MSVC, which QGC installs, not MinGW.
    """
    if arch.endswith("_cross_compiled"):
        return resolve_windows_host_arch(arch)
    try:
        return _HOST_DESKTOP_ARCHES[effective_host(host)]
    except KeyError:
        raise ValueError(f"No desktop Qt for host {host!r}") from None


@dataclass(frozen=True)
class QtRequest:
    host: str
    target: str
    version: str
    arch: str
    modules: tuple[str, ...] = ()
    # Base-package archive subset by name prefix (aqt --archives); None installs every archive.
    archives: tuple[str, ...] | None = None

    def __post_init__(self) -> None:
        parts = self.version.split(".")
        if len(parts) != 3 or not all(part.isdigit() for part in parts) or self._key < (6, 8):
            raise ValueError(f"QGC's Qt installer needs a Qt 6.8+ release, not {self.version!r}")
        if self.target != "android" and self.host not in _HOST_SEGMENTS:
            raise ValueError(f"Unknown Qt host {self.host!r} for target {self.target!r}")

    @property
    def _key(self) -> tuple[int, ...]:
        return tuple(int(part) for part in self.version.split("."))

    @property
    def _prefix(self) -> str:
        return f"qt.qt{self._key[0]}.{self.version.replace('.', '')}"

    @property
    def repo_dir(self) -> str:
        segment = "all_os" if self.target == "android" else _HOST_SEGMENTS[self.host]
        name = f"qt{self._key[0]}_{self.version.replace('.', '')}"
        return f"{REPOSITORY}/{segment}/{self.target}/{name}/{name}{self._suffix()}"

    def _suffix(self) -> str:
        if self.target == "android":
            return "_" + self.arch.removeprefix("android_")
        if self.target == "ios" and self.arch.startswith("ios_") and self._key >= (6, 12):
            return "_" + self.arch.removeprefix("ios_")
        if self.host == "windows" and self.target == "desktop" and self._key >= (6, 11):
            return "_" + self.arch.removeprefix("win64_")
        return ""

    def base_packages(self) -> tuple[str, ...]:
        names = (f"{self._prefix}.{self.arch}",)
        if self.arch.startswith("ios_"):
            names += (f"{self._prefix}.qtforios.{self.arch.removeprefix('ios_')}",)
        return names

    def module_packages(self, module: str) -> tuple[str, ...]:
        return (
            f"{self._prefix}.addons.{module}.{self.arch}",
            f"{self._prefix}.{module}.{self.arch}",
        )

    @property
    def addon_prefix(self) -> str:
        return f"{self._prefix}.addons."


@dataclass(frozen=True)
class Archive:
    package: str
    filename: str
    url_path: str  # relative to a mirror's root
    dest: str  # extraction directory relative to the output directory


@dataclass(frozen=True)
class Plan:
    archives: tuple[Archive, ...]
    unlisted_dependencies: tuple[tuple[str, str], ...]  # (requested module, addon it needs)


def _split(text: str | None) -> list[str]:
    return [item.strip() for item in (text or "").split(",") if item.strip()]


def _extract_dirs(package: Element) -> dict[str, str]:
    dirs = {}
    for operation in package.iterfind("Operations/Operation[@name='Extract']"):
        arguments = [argument.text or "" for argument in operation.iterfind("Argument")]
        if len(arguments) >= 2:
            dirs[arguments[1]] = arguments[0].removeprefix("@TargetDir@").strip("/")
    return dirs


def resolve_plan(request: QtRequest, updates_xml: bytes) -> Plan:
    """Select the base package and requested addons from a verified Updates.xml.

    Like aqt, ``request.archives`` filters only the base package, and addon dependencies are
    reported rather than installed.
    """
    packages = {
        node.findtext("Name", ""): node for node in ET.fromstring(updates_xml).iter("PackageUpdate")
    }
    base = next((packages[name] for name in request.base_packages() if name in packages), None)
    if base is None:
        raise LookupError(f"No {request.arch} base package in {request.repo_dir}/Updates.xml")
    selected = [base]
    missing = []
    for module in request.modules:
        package = next(
            (packages[n] for n in request.module_packages(module) if n in packages), None
        )
        if package is None:
            missing.append(module)
        else:
            selected.append(package)
    if missing:
        raise LookupError(f"Qt {request.version} {request.arch} has no modules {' '.join(missing)}")

    root = f"{request.version}/{resolve_arch_dir(request.arch)}"
    # aqt adds --modules to the --archives filter.
    keep = None if request.archives is None else {*request.archives, *request.modules}
    archives = []
    for package in selected:
        name, version = package.findtext("Name", ""), package.findtext("Version", "")
        dirs = _extract_dirs(package)
        for filename in _split(package.findtext("DownloadableArchives")):
            if package is base and keep is not None and filename.split("-", 1)[0] not in keep:
                continue
            dest = dirs.get(filename, "")
            if not (dest == root or dest.startswith(f"{root}/")) or ".." in dest.split("/"):
                raise LookupError(f"{name} extracts {filename} to {dest!r}, outside {root}")
            url_path = f"{request.repo_dir}/{name}/{version}{filename}"
            archives.append(Archive(name, filename, url_path, dest))
    return Plan(tuple(archives), _unlisted_dependencies(request, packages))


def _unlisted_dependencies(
    request: QtRequest, packages: dict[str, Element]
) -> tuple[tuple[str, str], ...]:
    # Addon-to-addon dependencies sit on the arch-less package (qtquick3d -> qtquicktimeline).
    pairs = []
    for module in request.modules:
        meta = packages.get(request.addon_prefix + module)
        for dependency in _split(meta.findtext("Dependencies") if meta is not None else None):
            needed = dependency.removeprefix(request.addon_prefix)
            if dependency.startswith(request.addon_prefix) and needed not in request.modules:
                pairs.append((module, needed))
    return tuple(pairs)
