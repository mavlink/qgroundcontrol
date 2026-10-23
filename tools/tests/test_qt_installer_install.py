"""Tests for tools/setup/qt_installer/{download,install}.py.

End-to-end installs read a repository published under a ``file://`` root with real 7z archives
built by ``cmake -E tar``, so they exercise the download, checksum and extraction code offline.
"""

from __future__ import annotations

import hashlib
import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile
from email.message import Message
from pathlib import Path
from urllib.error import HTTPError

import pytest
from common.net import ChecksumMismatchError
from setup.qt_installer import download, install
from setup.qt_installer.install import ensure_host_desktop, extract, find_cmake
from setup.qt_installer.repo import QtRequest

from ._helpers import write_files

pytestmark = pytest.mark.windows

POSIX_ONLY = pytest.mark.skipif(sys.platform == "win32", reason="POSIX modes and symlinks")
SHA = "a" * 64
VERSION = "6.12.0-0-202609280346"


def _sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _http_error(url: str, code: int) -> HTTPError:
    return HTTPError(url, code, "error", Message(), None)


def _make_archive(
    path: Path, files: dict[str, str], *, executable: str = "", extra: Path | None = None
) -> bytes:
    """Write a 7z archive of *files* (plus *extra*'s tree, symlinks kept) to *path*."""
    # Build in a short temporary directory: Windows rejects a working directory longer than
    # MAX_PATH, and the mirrored repository paths under pytest's tmp_path exceed it.
    work = Path(tempfile.mkdtemp(prefix="qa-"))
    try:
        source = work / "src"
        if extra is not None:
            shutil.copytree(extra, source, symlinks=True)
        write_files(source, files)
        if executable:
            (source / executable).chmod(0o755)
        entries = sorted(entry.name for entry in source.iterdir())
        archive = work / "archive.7z"
        subprocess.run(
            [find_cmake(), "-E", "tar", "cf", str(archive), "--format=7zip", *entries],
            cwd=source,
            check=True,
            capture_output=True,
        )
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(archive, path)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    return path.read_bytes()


class TestTrustedChecksums:
    def test_reads_the_digest_from_download_qt_io(self, monkeypatch) -> None:
        urls = []

        def fetch(url: str, **_kwargs) -> bytes:
            urls.append(url)
            return f"{SHA.upper()}  Updates.xml\n".encode()

        monkeypatch.setattr(download, "fetch_bytes", fetch)
        assert download.trusted_sha256("online/x/Updates.xml") == SHA
        assert urls == ["https://download.qt.io/online/x/Updates.xml.sha256"]

    @pytest.mark.parametrize("body", [b"", b"not-a-digest  file.7z\n", b"abc123"])
    def test_malformed_digest_fails(self, monkeypatch, body: bytes) -> None:
        monkeypatch.setattr(download, "fetch_bytes", lambda url, **_kwargs: body)
        with pytest.raises(RuntimeError, match="Malformed SHA-256"):
            download.trusted_sha256("online/x/a.7z")

    def test_missing_digest_fails(self, monkeypatch) -> None:
        def fetch(url: str, **_kwargs) -> bytes:
            raise _http_error(url, 404)

        monkeypatch.setattr(download, "fetch_bytes", fetch)
        with pytest.raises(RuntimeError, match=r"no SHA-256 for online/x/a\.7z \(HTTP 404\)"):
            download.trusted_sha256("online/x/a.7z")


class TestUpdatesXml:
    @staticmethod
    def _serve(monkeypatch, xml: bytes, digest: str) -> None:
        responses = {
            "https://download.qt.io/r/Updates.xml": xml,
            "https://download.qt.io/r/Updates.xml.sha256": f"{digest}  Updates.xml\n".encode(),
        }
        monkeypatch.setattr(download, "fetch_bytes", lambda url, **_kwargs: responses[url])

    def test_verified_metadata_is_returned(self, monkeypatch) -> None:
        self._serve(monkeypatch, b"<Updates/>", _sha256(b"<Updates/>"))
        assert download.fetch_updates_xml("r") == b"<Updates/>"

    def test_checksum_mismatch_fails(self, monkeypatch) -> None:
        self._serve(monkeypatch, b"<Updates/>", SHA)
        with pytest.raises(RuntimeError, match=r"SHA-256 mismatch for r/Updates\.xml"):
            download.fetch_updates_xml("r")

    def test_missing_repository_names_the_url(self, monkeypatch) -> None:
        def fetch(url: str, **_kwargs) -> bytes:
            raise _http_error(url, 404)

        monkeypatch.setattr(download, "fetch_bytes", fetch)
        with pytest.raises(RuntimeError, match=r"Cannot fetch https://download\.qt\.io/r/Updates"):
            download.fetch_updates_xml("r")


class TestDownloadArchive:
    def test_falls_back_to_the_next_mirror(self, monkeypatch, tmp_path: Path) -> None:
        calls = []

        def fake_download(url: str, dest: Path, *, expected_sha256: str) -> None:
            calls.append((url, expected_sha256))
            if url.startswith("https://download.qt.io/"):
                raise ChecksumMismatchError("stale mirror")
            dest.write_bytes(b"ok")

        monkeypatch.setattr(download, "download_file", fake_download)
        download.download_archive("online/a.7z", tmp_path / "a.7z", SHA)
        assert calls == [
            ("https://download.qt.io/online/a.7z", SHA),
            ("https://master.qt.io/online/a.7z", SHA),
        ]
        assert (tmp_path / "a.7z").read_bytes() == b"ok"

    def test_reports_every_mirror_when_all_fail(self, monkeypatch, tmp_path: Path) -> None:
        def fake_download(url: str, dest: Path, *, expected_sha256: str) -> None:
            raise _http_error(url, 404) if "master" in url else ChecksumMismatchError("bad")

        monkeypatch.setattr(download, "download_file", fake_download)
        with pytest.raises(RuntimeError, match=r"No mirror served online/a\.7z") as error:
            download.download_archive("online/a.7z", tmp_path / "a.7z", SHA)
        for mirror in download.MIRRORS:
            assert f"{mirror}: " in str(error.value)


class TestFindCmake:
    def test_prefers_path(self, monkeypatch) -> None:
        monkeypatch.setattr(install.shutil, "which", lambda name: f"/usr/bin/{name}")
        assert find_cmake() == "/usr/bin/cmake"

    def test_falls_back_to_the_tools_environment(self, monkeypatch, tmp_path: Path) -> None:
        cmake = tmp_path / "cmake"
        cmake.write_text("")
        monkeypatch.setattr(install.shutil, "which", lambda _name: None)
        monkeypatch.setattr(install, "executable", lambda _name: cmake)
        assert find_cmake() == str(cmake)

    def test_explains_how_to_get_cmake(self, monkeypatch, tmp_path: Path) -> None:
        monkeypatch.setattr(install.shutil, "which", lambda _name: None)
        monkeypatch.setattr(install, "executable", lambda _name: tmp_path / "missing")
        with pytest.raises(RuntimeError, match="--installer aqt"):
            find_cmake()


class TestExtract:
    def test_extracts_a_7z_archive(self, tmp_path: Path) -> None:
        archive = tmp_path / "a.7z"
        _make_archive(archive, {"bin/tool": "#!/bin/sh\n", "lib/cmake/a.txt": "hi\n"})

        extract(archive, tmp_path / "out" / "lib", find_cmake())

        assert (tmp_path / "out/lib/lib/cmake/a.txt").read_text() == "hi\n"
        assert (tmp_path / "out/lib/bin/tool").is_file()

    @POSIX_ONLY
    def test_keeps_symlinks_and_execute_bits(self, tmp_path: Path) -> None:
        source = tmp_path / "a.7z.src"
        (source / "lib/QtCore.framework/Versions/A").mkdir(parents=True)
        (source / "lib/QtCore.framework/Versions/Current").symlink_to("A")
        archive = tmp_path / "a.7z"
        _make_archive(archive, {"bin/tool": "#!/bin/sh\n"}, executable="bin/tool", extra=source)

        extract(archive, tmp_path / "out", find_cmake())

        assert os.readlink(tmp_path / "out/lib/QtCore.framework/Versions/Current") == "A"
        assert stat.S_IMODE((tmp_path / "out/bin/tool").stat().st_mode) & 0o111

    def test_corrupt_archive_fails(self, tmp_path: Path) -> None:
        archive = tmp_path / "bad.7z"
        archive.write_bytes(b"7z\xbc\xaf\x27\x1c garbage")
        with pytest.raises(RuntimeError, match=r"cmake -E tar xf bad\.7z failed"):
            extract(archive, tmp_path / "out", find_cmake())


class Repository:
    """A ``file://`` Qt repository holding one combination's Updates.xml, archives and sums."""

    def __init__(self, root: Path, request: QtRequest) -> None:
        self.root, self.request = root, request
        self.directory = root / request.repo_dir
        self.packages: list[str] = []

    def add(
        self, name: str, archives: dict[str, tuple[str, dict[str, str]]], deps: str = ""
    ) -> None:
        """Publish package *name*; *archives* maps a file name to (extract dir, contents)."""
        operations = "".join(
            f'<Operation name="Extract"><Argument>@TargetDir@/{dest}</Argument>'
            f"<Argument>{filename}</Argument></Operation>"
            for filename, (dest, _) in archives.items()
        )
        self.packages.append(
            f"<PackageUpdate><Name>{name}</Name><Version>{VERSION}</Version>"
            f"<Dependencies>{deps}</Dependencies>"
            f"<DownloadableArchives>{', '.join(archives)}</DownloadableArchives>"
            f"<Operations>{operations}</Operations></PackageUpdate>"
        )
        for filename, (_, files) in archives.items():
            path = self.directory / name / f"{VERSION}{filename}"
            data = _make_archive(path, files)
            path.with_name(f"{path.name}.sha256").write_text(f"{_sha256(data)}  {path.name}\n")

    def publish(self, monkeypatch) -> None:
        xml = f"<Updates>{''.join(self.packages)}</Updates>".encode()
        (self.directory / "Updates.xml").write_bytes(xml)
        (self.directory / "Updates.xml.sha256").write_text(f"{_sha256(xml)}  Updates.xml\n")
        url = self.root.as_uri()
        monkeypatch.setattr(download, "TRUSTED_URL", url)
        monkeypatch.setattr(download, "MIRRORS", (url,))


def _desktop_repository(tmp_path: Path, request: QtRequest) -> Repository:
    repository = Repository(tmp_path / "repo", request)
    root = "6.12.0/gcc_64"
    repository.add(
        "qt.qt6.6120.linux_gcc_64",
        {
            "qtbase-Linux.7z": (
                root,
                {
                    "bin/qmake": "#!/bin/sh\n",
                    "lib/pkgconfig/Qt6Core.pc": "prefix=/home/qt/work/install\n",
                    "include/QtCore/qglobal.h": "\n",
                },
            ),
            "icu-linux.7z": (f"{root}/lib", {"libicudata.so.73": "icu\n"}),
        },
    )
    repository.add("qt.qt6.6120.addons.qtlocation", {}, deps="qt.qt6.6120.addons.qtpositioning")
    repository.add(
        "qt.qt6.6120.addons.qtlocation.linux_gcc_64",
        {"qtlocation-Linux.7z": (root, {"lib/libQt6Location.so": "location\n"})},
    )
    return repository


class TestInstall:
    def test_installs_patches_and_records_a_desktop_qt(
        self, monkeypatch, tmp_path: Path, capsys
    ) -> None:
        request = QtRequest("linux", "desktop", "6.12.0", "linux_gcc_64", ("qtlocation",))
        _desktop_repository(tmp_path, request).publish(monkeypatch)
        outdir = tmp_path / "Qt"
        umask = os.umask(0o002)
        try:
            prefix = install.install(request, outdir, jobs=2)
        finally:
            os.umask(umask)

        assert prefix == outdir / "6.12.0" / "gcc_64"
        assert (prefix / "lib/libicudata.so.73").read_text() == "icu\n"
        assert (prefix / "lib/libQt6Location.so").read_text() == "location\n"
        assert (prefix / "lib/pkgconfig/Qt6Core.pc").read_text() == f"prefix={prefix}\n"
        assert (prefix / "bin/qt.conf").is_file()
        assert (prefix / ".qgc-modules").read_text() == "qtlocation\n"
        manifest = json.loads((prefix / ".qgc-qt-manifest.json").read_text())
        assert manifest["installer"] == "qgc"
        assert manifest["patched"] == ["bin/qt.conf", "lib/pkgconfig/Qt6Core.pc"]
        assert [(a["file"], a["dest"]) for a in manifest["archives"]] == [
            ("qtbase-Linux.7z", "6.12.0/gcc_64"),
            ("icu-linux.7z", "6.12.0/gcc_64/lib"),
            ("qtlocation-Linux.7z", "6.12.0/gcc_64"),
        ]
        published = tmp_path / "repo" / request.repo_dir / "qt.qt6.6120.linux_gcc_64"
        expected = _sha256((published / f"{VERSION}icu-linux.7z").read_bytes())
        assert manifest["archives"][1]["sha256"] == expected
        assert not list(outdir.glob(".qgc-qt-download-*"))
        assert "qtlocation depends on qtpositioning" in capsys.readouterr().err
        if sys.platform != "win32":
            # A fixed umask makes the modes independent of the caller's, like aqt's.
            assert stat.S_IMODE((prefix / "include/QtCore").stat().st_mode) == 0o755
            assert stat.S_IMODE((prefix / "bin/qt.conf").stat().st_mode) == 0o644

    def test_archive_subset_without_qtbase_is_left_unpatched(self, monkeypatch, tmp_path) -> None:
        request = QtRequest("linux", "desktop", "6.12.0", "linux_gcc_64", archives=("icu",))
        _desktop_repository(tmp_path, request).publish(monkeypatch)

        prefix = install.install(request, tmp_path / "Qt", jobs=1)

        assert sorted(path.name for path in prefix.iterdir()) == [".qgc-qt-manifest.json", "lib"]
        manifest = json.loads((prefix / ".qgc-qt-manifest.json").read_text())
        assert (manifest["archives_filter"], manifest["patched"]) == (["icu"], [])

    def test_checksum_mismatch_fails_without_leaving_downloads(self, monkeypatch, tmp_path) -> None:
        request = QtRequest("linux", "desktop", "6.12.0", "linux_gcc_64")
        repository = _desktop_repository(tmp_path, request)
        repository.publish(monkeypatch)
        sums = (
            repository.directory / "qt.qt6.6120.linux_gcc_64" / f"{VERSION}qtbase-Linux.7z.sha256"
        )
        sums.write_text(f"{SHA}  qtbase.7z\n")
        outdir = tmp_path / "Qt"

        with pytest.raises(RuntimeError, match="No mirror served"):
            install.install(request, outdir, jobs=1)
        assert not list(outdir.glob(".qgc-qt-download-*"))
        assert not (outdir / "6.12.0/gcc_64/.qgc-qt-manifest.json").exists()

    def test_android_points_host_tools_at_the_desktop_qt(self, monkeypatch, tmp_path) -> None:
        request = QtRequest("linux", "android", "6.12.0", "android_arm64_v8a")
        repository = Repository(tmp_path / "repo", request)
        repository.add(
            "qt.qt6.6120.android_arm64_v8a",
            {
                "qtbase-Android.7z": (
                    "6.12.0/android_arm64_v8a",
                    {
                        "bin/qmake": "/Users/qt/work/install/bin/qmake6 $*\n",
                        "bin/target_qt.conf": "HostPrefix=../../\nHostData=target\n",
                    },
                )
            },
        )
        repository.publish(monkeypatch)
        outdir = tmp_path / "Qt"

        prefix = install.install(request, outdir, jobs=1)

        assert (prefix / "bin/qmake").read_text() == f"{outdir}/6.12.0/gcc_64/bin/qmake6 $*\n"
        assert (prefix / "bin/target_qt.conf").read_text() == (
            "HostPrefix=../../gcc_64\nHostData=../android_arm64_v8a\n"
        )
        assert not (prefix / "bin/qt.conf").exists()


class TestEnsureHostDesktop:
    @pytest.fixture
    def installs(self, monkeypatch) -> list[QtRequest]:
        calls: list[QtRequest] = []
        monkeypatch.setattr(install, "install", lambda request, _outdir: calls.append(request))
        return calls

    def test_installs_the_missing_desktop_with_the_same_modules(self, tmp_path, installs) -> None:
        request = QtRequest("linux", "android", "6.12.0", "android_armv7", ("qtlocation",))
        ensure_host_desktop(request, tmp_path)
        assert installs == [
            QtRequest("linux", "desktop", "6.12.0", "linux_gcc_64", ("qtlocation",), None)
        ]

    def test_reuses_an_installed_desktop(self, tmp_path, installs) -> None:
        write_files(tmp_path, {"6.12.0/msvc2022_64/bin/qmake.exe": ""})
        ensure_host_desktop(
            QtRequest("windows", "desktop", "6.12.0", "win64_msvc2022_arm64_cross_compiled"),
            tmp_path,
        )
        assert installs == []

    def test_desktop_targets_need_no_host_qt(self, tmp_path, installs) -> None:
        ensure_host_desktop(QtRequest("linux", "desktop", "6.12.0", "linux_gcc_64"), tmp_path)
        assert installs == []
