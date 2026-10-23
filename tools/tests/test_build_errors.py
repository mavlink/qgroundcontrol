"""Tests for build_errors.py, on output captured from real configure and build failures."""

from __future__ import annotations

import textwrap
from typing import TYPE_CHECKING

import pytest
from qgc_tools.build_errors import (
    MAX_BLOCK_LINES,
    MAX_ERRORS,
    MAX_LINE_CHARS,
    Diagnostic,
    cmake_errors,
    parse_build_failure,
    repo_path,
)

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows


def _log(text: str) -> str:
    return textwrap.dedent(text).lstrip("\n")


# Windows (MSVC) configure of zstd under CPM; the step showed only "Process completed with exit
# code 1." and a "Configure completed in 36s" notice.
ZSTD_ASM_CONFIGURE = _log("""
    -- liblzma support enabled
    -- The ASM compiler identification is unknown
    -- Didn't find assembler
    CMake Error at .cache/CPM/zstd/dfd2/build/cmake/CMakeLists.txt:36 (project):
      No CMAKE_ASM_COMPILER could be found.

      Tell CMake where to find the compiler by setting either the environment
      variable "ASM" or the CMake cache entry CMAKE_ASM_COMPILER to the full path
      to the compiler, or to the compiler name if it is in the PATH.


    -- Configuring incomplete, errors occurred!
""")

VALIJSON_CLONE_CONFIGURE = _log("""
    -- CPM: Adding package valijson@1.0.6 (v1.0.6 at /home/runner/_work/qgroundcontrol/qgroundcontrol/.cache/CPM/valijson/3a1b)
    Cloning into 'valijson-src'...
    fatal: unable to access 'https://github.com/tristanpenman/valijson.git/': Could not resolve host: github.com
    -- Had to git clone more than once: 3 times.
    CMake Error at /usr/local/share/cmake-4.1/Modules/ExternalProject/shared_internal_commands.cmake:943 (message):
      Failed to clone repository:
      'https://github.com/tristanpenman/valijson.git'
    Call Stack (most recent call first):
      /usr/local/share/cmake-4.1/Modules/FetchContent.cmake:1564 (__FetchContent_doPopulation)
      cmake/modules/CPM.cmake:1131 (FetchContent_MakeAvailable)
      CMakeLists.txt:180 (CPMAddPackage)


    -- Configuring incomplete, errors occurred!
""")


def test_cmake_error_at_a_file_keeps_its_location_message_and_block() -> None:
    (error,) = cmake_errors(ZSTD_ASM_CONFIGURE)
    assert (error.title, error.file, error.line, error.col) == (
        "CMake Error",
        ".cache/CPM/zstd/dfd2/build/cmake/CMakeLists.txt",
        36,
        None,
    )
    assert error.message.splitlines() == [
        "No CMAKE_ASM_COMPILER could be found.",
        "",
        "Tell CMake where to find the compiler by setting either the environment",
        'variable "ASM" or the CMake cache entry CMAKE_ASM_COMPILER to the full path',
        "to the compiler, or to the compiler name if it is in the PATH.",
    ]
    assert error.excerpt.splitlines()[0] == (
        "CMake Error at .cache/CPM/zstd/dfd2/build/cmake/CMakeLists.txt:36 (project):"
    )
    assert "Configuring incomplete" not in error.excerpt


def test_cmake_error_message_stops_at_the_call_stack_that_the_excerpt_keeps() -> None:
    (error,) = cmake_errors(VALIJSON_CLONE_CONFIGURE)
    assert error.file.endswith("ExternalProject/shared_internal_commands.cmake")
    assert error.line == 943
    assert error.message == (
        "Failed to clone repository:\n'https://github.com/tristanpenman/valijson.git'"
    )
    assert error.excerpt.splitlines()[-1] == "  CMakeLists.txt:180 (CPMAddPackage)"


def test_cmake_errors_without_a_line_or_location() -> None:
    errors = cmake_errors(
        _log("""
        CMake Error in src/CMakeLists.txt:
          Target "QGroundControl" INTERFACE_INCLUDE_DIRECTORIES property contains
          path:

            "/home/runner/_work/qgroundcontrol/qgroundcontrol/src/missing"

          which is prefixed in the source directory.


        CMake Error: The source directory "/home/runner/_work/missing" does not exist.
        Specify --help for usage, or press the help button on the CMake GUI.
        CMake Warning (dev) at CMakeLists.txt:3 (project):
          Not an error.
        """)
    )
    assert [(error.file, error.line) for error in errors] == [
        ("src/CMakeLists.txt", None),
        ("", None),
    ]
    assert errors[0].message.splitlines()[-1] == "which is prefixed in the source directory."
    assert errors[1].message == 'The source directory "/home/runner/_work/missing" does not exist.'


# Linux GCC unit build (job 118648116370): Ninja 1.13 FAILED edge, command, then GCC's output.
GCC_BUILD = _log("""
    [2224/2441] Building CXX object CMakeFiles/QGroundControl.dir/test/MAVLink/HealthAndArmingCheckReportTest.cc.o
    FAILED: [code=1] CMakeFiles/QGroundControl.dir/test/MAVLink/HealthAndArmingCheckReportTest.cc.o
    /home/runner/_work/_temp/build/ccache-launcher /usr/bin/c++ -DQGC_DAILY_BUILD -c ../../qgroundcontrol/qgroundcontrol/test/MAVLink/HealthAndArmingCheckReportTest.cc
    ../../qgroundcontrol/qgroundcontrol/test/MAVLink/HealthAndArmingCheckReportTest.cc: In member function \u2018void HealthAndArmingCheckReportTest::_testFirstReportInUnmappedMode()\u2019:
    ../../qgroundcontrol/qgroundcontrol/test/MAVLink/HealthAndArmingCheckReportTest.cc:83:31: error: no matching function for call to \u2018EventHandler::EventHandler(std::nullptr_t, QString)\u2019
       83 |         MAV_COMP_ID_AUTOPILOT1);
          |                               ^
    In file included from ../../qgroundcontrol/qgroundcontrol/test/MAVLink/HealthAndArmingCheckReportTest.cc:7:
    ../../qgroundcontrol/qgroundcontrol/src/MAVLink/LibEvents/EventHandler.h:30:5: note: candidate: \u2018EventHandler::EventHandler(QObject*, const QString&)\u2019
       30 |     EventHandler(QObject* parent, const QString& profile, handle_event_f handleEventCB,
          |     ^~~~~~~~~~~~
    ../../qgroundcontrol/qgroundcontrol/test/MAVLink/HealthAndArmingCheckReportTest.cc:144:31: error: no matching function for call to \u2018EventHandler::EventHandler(std::nullptr_t, QString)\u2019
    [2225/2441] Building CXX object CMakeFiles/QGroundControl.dir/test/MAVLink/ImageProtocolManagerTest.cc.o
    [2229/2441] Building CXX object CMakeFiles/QGroundControl.dir/test/GPS/Core/Protocols/UBX/UBXProtocolTest.cc.o
    ninja: build stopped: subcommand failed.
""")


def test_gcc_errors_and_the_failed_ninja_target() -> None:
    failure = parse_build_failure(GCC_BUILD)
    assert failure.failed_targets == [
        "CMakeFiles/QGroundControl.dir/test/MAVLink/HealthAndArmingCheckReportTest.cc.o"
    ]
    source = "../../qgroundcontrol/qgroundcontrol/test/MAVLink/HealthAndArmingCheckReportTest.cc"
    assert [(e.title, e.file, e.line, e.col) for e in failure.errors] == [
        ("Compile error", source, 83, 31),
        ("Compile error", source, 144, 31),
    ]
    assert failure.errors[0].message == (
        "no matching function for call to \u2018EventHandler::EventHandler(std::nullptr_t, QString)\u2019"
    )
    assert failure.error_count == 2


@pytest.mark.parametrize(
    ("line", "expected"),
    [
        (
            "../../qgroundcontrol/qgroundcontrol/test/GPS/GpsTest.cc:12:10: fatal error: "
            "GpsTestHelpers.h: No such file or directory",
            Diagnostic(
                "Compile error",
                "GpsTestHelpers.h: No such file or directory",
                "../../qgroundcontrol/qgroundcontrol/test/GPS/GpsTest.cc",
                12,
                10,
            ),
        ),
        (
            "/Users/runner/work/qgroundcontrol/qgroundcontrol/test/GPS/GpsTest.cc:12:10: fatal "
            "error: 'GpsTestHelpers.h' file not found",
            Diagnostic(
                "Compile error",
                "'GpsTestHelpers.h' file not found",
                "/Users/runner/work/qgroundcontrol/qgroundcontrol/test/GPS/GpsTest.cc",
                12,
                10,
            ),
        ),
        (
            r"C:\a\qgroundcontrol\qgroundcontrol\src\Vehicle\Vehicle.cc(120): error C2065: "
            "'foo': undeclared identifier",
            Diagnostic(
                "Compile error",
                "C2065: 'foo': undeclared identifier",
                r"C:\a\qgroundcontrol\qgroundcontrol\src\Vehicle\Vehicle.cc",
                120,
            ),
        ),
        (
            r"C:\Program Files (x86)\Windows Kits\10\include\ucrt\stdio.h(12,5): fatal error "
            "C1083: Cannot open include file: 'foo.h': No such file or directory",
            Diagnostic(
                "Compile error",
                "C1083: Cannot open include file: 'foo.h': No such file or directory",
                r"C:\Program Files (x86)\Windows Kits\10\include\ucrt\stdio.h",
                12,
                5,
            ),
        ),
        (
            r"..\..\src\Vehicle\Vehicle.cc(120,5): error: use of undeclared identifier 'foo'",
            Diagnostic(
                "Compile error",
                "use of undeclared identifier 'foo'",
                r"..\..\src\Vehicle\Vehicle.cc",
                120,
                5,
            ),
        ),
        (
            'Vehicle.cc.obj : error LNK2019: unresolved external symbol "void __cdecl foo(void)" '
            "(?foo@@YAXXZ) referenced in function main",
            Diagnostic(
                "Link error",
                'Vehicle.cc.obj: LNK2019: unresolved external symbol "void __cdecl foo(void)" '
                "(?foo@@YAXXZ) referenced in function main",
            ),
        ),
        (
            "LINK : fatal error LNK1104: cannot open file 'Qt6Core.lib'",
            Diagnostic("Link error", "LINK: LNK1104: cannot open file 'Qt6Core.lib'"),
        ),
        (
            "main.cc:(.text.startup+0x15): undefined reference to `foo()'",
            Diagnostic("Link error", "main.cc: undefined reference to `foo()'"),
        ),
        (
            "ld.lld: error: undefined symbol: foo()",
            Diagnostic("Link error", "ld.lld: undefined symbol: foo()"),
        ),
        (
            "mold: error: undefined symbol: foo()",
            Diagnostic("Link error", "mold: undefined symbol: foo()"),
        ),
        (
            "c++: error: unrecognized command-line option '-fbogus'",
            Diagnostic("Build error", "c++: unrecognized command-line option '-fbogus'"),
        ),
        (
            "ninja: error: 'src/Gone.cc', needed by 'CMakeFiles/QGC.dir/src/Gone.cc.o', missing "
            "and no known rule to make it",
            Diagnostic(
                "Ninja error",
                "'src/Gone.cc', needed by 'CMakeFiles/QGC.dir/src/Gone.cc.o', missing and no "
                "known rule to make it",
            ),
        ),
        (
            "/home/runner/_work/_temp/build/android-build/src/org/mavlink/qgroundcontrol/"
            "QGCActivity.java:42: error: cannot find symbol",
            Diagnostic(
                "Compile error",
                "cannot find symbol",
                "/home/runner/_work/_temp/build/android-build/src/org/mavlink/qgroundcontrol/"
                "QGCActivity.java",
                42,
            ),
        ),
    ],
)
def test_compiler_and_linker_error_lines(line: str, expected: Diagnostic) -> None:
    (error,) = parse_build_failure(f"[1/2] Building\n{line}\n").errors
    assert (error.title, error.message, error.file, error.line, error.col) == (
        expected.title,
        expected.message,
        expected.file,
        expected.line,
        expected.col,
    )
    assert error.excerpt == line


@pytest.mark.parametrize(
    "line",
    [
        "/home/runner/_work/_temp/build/android-build/src/org/libsdl/app/HIDDeviceManager.java:69:"
        " warning: [deprecation] <T>getParcelableExtra(String) in Intent has been deprecated",
        "../../src/Vehicle/Vehicle.cc:10:5: warning: unused variable 'x' [-Wunused-variable]",
        "../../src/Vehicle/Vehicle.cc:30:5: note: candidate: 'Vehicle::Vehicle()'",
        "-- Performing Test HAVE_STDATOMIC - Failed",
        "[42/2441] Building CXX object CMakeFiles/QGroundControl.dir/src/error.cc.o",
        "    error: indented text is part of an earlier message",
    ],
)
def test_warnings_notes_and_progress_are_not_errors(line: str) -> None:
    assert parse_build_failure(f"{line}\n").errors == []


def test_gnu_link_failure_reports_the_reference_not_the_exit_status() -> None:
    failure = parse_build_failure(
        _log("""
        [2441/2441] Linking CXX executable Debug/QGroundControl
        FAILED: [code=1] Debug/QGroundControl
        : && /usr/bin/c++ -g1 CMakeFiles/QGroundControl.dir/src/main.cc.o -o Debug/QGroundControl
        /usr/bin/ld: CMakeFiles/QGroundControl.dir/src/main.cc.o: in function `main':
        main.cc:(.text.startup+0x15): undefined reference to `foo()'
        collect2: error: ld returned 1 exit status
        ninja: build stopped: subcommand failed.
        """)
    )
    assert failure.failed_targets == ["Debug/QGroundControl"]
    assert [error.message for error in failure.errors] == [
        "main.cc: undefined reference to `foo()'"
    ]


def test_msvc_link_failure_reports_each_linker_error() -> None:
    failure = parse_build_failure(
        _log(r"""
        [3/3] Linking CXX executable Release\QGroundControl.exe
        FAILED: [code=4294967295] Release/QGroundControl.exe
        C:\WINDOWS\system32\cmd.exe /C "cd . && cmake.exe -E vs_link_exe --msvc-ver=1944 -- link.exe /nologo"
        LINK: command "link.exe /nologo @CMakeFiles\QGroundControl.rsp" failed (exit code 1120) with the following output:
        Vehicle.cc.obj : error LNK2019: unresolved external symbol "void __cdecl foo(void)" (?foo@@YAXXZ) referenced in function main
        Release\QGroundControl.exe : fatal error LNK1120: 1 unresolved externals
        ninja: build stopped: subcommand failed.
        """)
    )
    assert failure.failed_targets == ["Release/QGroundControl.exe"]
    assert [error.message.split(":")[1].strip() for error in failure.errors] == [
        "LNK2019",
        "LNK1120",
    ]
    assert {error.title for error in failure.errors} == {"Link error"}


# Android (cmake --build runs androiddeployqt, which runs Gradle).
GRADLE_BUILD = _log("""
    > Task :compileReleaseJavaWithJavac
    /home/runner/_work/_temp/build/android-build/src/org/libsdl/app/HIDDeviceManager.java:69: warning: [deprecation] <T>getParcelableExtra(String) in Intent has been deprecated
                    UsbDevice usbDevice = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                                                ^
    /home/runner/_work/_temp/build/android-build/src/org/mavlink/qgroundcontrol/QGCActivity.java:42: error: cannot find symbol
            QGCUsbSerialManager.missing();
                               ^
      symbol:   method missing()
      location: class QGCUsbSerialManager
    1 error
    1 warning

    > Task :compileReleaseJavaWithJavac FAILED

    FAILURE: Build failed with an exception.

    * What went wrong:
    Execution failed for task ':compileReleaseJavaWithJavac'.
    > Compilation failed; see the compiler error output for details.

    * Try:
    > Run with --info option to get more log output.
    > Run with --scan to get full insights.

    BUILD FAILED in 41s
    Building the android package failed!
      -- For more information, run this command with --verbose.
    FAILED: [code=14] android-build/QGroundControl.apk
    cd /home/runner/_work/_temp/build && /opt/Qt/6.10.1/gcc_64/bin/androiddeployqt --input android-QGroundControl-deployment-settings.json
    ninja: build stopped: subcommand failed.
""")


def test_gradle_failure_reports_javac_error_and_what_went_wrong() -> None:
    failure = parse_build_failure(GRADLE_BUILD)
    assert failure.failed_targets == ["android-build/QGroundControl.apk"]
    javac, gradle = failure.errors
    assert (javac.title, javac.line, javac.message) == ("Compile error", 42, "cannot find symbol")
    assert gradle.title == "Gradle build failed"
    assert gradle.message == (
        "Execution failed for task ':compileReleaseJavaWithJavac'.\n"
        "> Compilation failed; see the compiler error output for details."
    )
    assert gradle.excerpt.startswith("* What went wrong:\n")


def test_failed_edge_without_a_recognized_error_reports_its_output() -> None:
    failure = parse_build_failure(
        _log("""
        [12/40] Automatic MOC and UIC for target QGroundControl
        FAILED: [code=1] QGroundControl_autogen/timestamp
        cd /home/runner/_work/_temp/build && cmake -E cmake_autogen AutogenInfo.json Debug
        AutoMoc subprocess error
        ------------------------
        The moc process failed to compile
          "SRC:/src/Vehicle/Vehicle.h"
        [13/40] Building CXX object x.o
        ninja: build stopped: subcommand failed.
        """)
    )
    (error,) = failure.errors
    assert error.title == "Build step failed"
    assert error.message.splitlines() == [
        "QGroundControl_autogen/timestamp",
        "AutoMoc subprocess error",
        "------------------------",
        "The moc process failed to compile",
        '  "SRC:/src/Vehicle/Vehicle.h"',
    ]


def test_ninja_failed_line_drops_its_trailing_space() -> None:
    failure = parse_build_failure("FAILED: [code=1] src/A.cc.o \n/usr/bin/c++ -c a.cc\n")
    assert failure.failed_targets == ["src/A.cc.o"]


def test_exit_status_summary_reports_only_when_nothing_else_does() -> None:
    (error,) = parse_build_failure(
        "clang++: error: linker command failed with exit code 1\n"
    ).errors
    assert error.message == "clang++: linker command failed with exit code 1"


def test_cmake_regeneration_and_xcode_failures() -> None:
    failure = parse_build_failure(
        _log("""
        [0/2] Re-checking globbed directories...
        [1/2] Re-running CMake...
        CMake Error at src/CMakeLists.txt:12 (target_sources):
          Cannot find source file:

            Gone.cc

        The following build commands failed:
        \tCompileC /Users/runner/build/QGC.build/Vehicle.o /Users/runner/src/Vehicle.cc normal arm64 c++
        \tBuilding project QGroundControl with scheme QGroundControl
        (2 failures)
        """)
    )
    (error,) = failure.errors
    assert (error.title, error.file, error.line) == ("CMake Error", "src/CMakeLists.txt", 12)
    assert failure.failed_targets == [
        "CompileC /Users/runner/build/QGC.build/Vehicle.o /Users/runner/src/Vehicle.cc normal arm64 c++",
        "Building project QGroundControl with scheme QGroundControl",
    ]


def test_errors_are_deduplicated_capped_and_counted() -> None:
    lines = [
        f"../../src/Unity{n % 15}.cc:{n % 15}:1: error: 'x' was not declared" for n in range(30)
    ]
    failure = parse_build_failure("\n".join(lines))
    assert len(failure.errors) == MAX_ERRORS
    assert failure.error_count == 15
    assert failure.errors[0].file == "../../src/Unity0.cc"


def test_long_lines_and_blocks_are_clipped() -> None:
    (error,) = parse_build_failure(f"a.cc:1:1: error: {'x' * 2000}\n").errors
    assert len(error.message) == MAX_LINE_CHARS
    assert error.message.endswith("…")

    body = "\n".join(f"  line {n}" for n in range(40))
    (error,) = cmake_errors(f"CMake Error at CMakeLists.txt:1 (message):\n{body}\n")
    assert error.message.splitlines()[-1] == f"… {40 - MAX_BLOCK_LINES} more lines"


def test_ansi_colors_do_not_hide_errors() -> None:
    line = (
        "\x1b[1m../../src/A.cc:3:4: \x1b[0;1;31merror: \x1b[0m\x1b[1munknown type name 'Foo'\x1b[0m"
    )
    (error,) = parse_build_failure(line).errors
    assert (error.file, error.message) == ("../../src/A.cc", "unknown type name 'Foo'")


class TestRepoPath:
    def test_build_relative_path_maps_into_the_checkout(self, tmp_path: Path) -> None:
        repo = tmp_path / "qgroundcontrol" / "qgroundcontrol"
        build = tmp_path / "_temp" / "build"
        source = "../../qgroundcontrol/qgroundcontrol/test/MAVLink/Test.cc"
        assert repo_path(source, bases=(build, repo), repo_root=repo) == "test/MAVLink/Test.cc"

    def test_relative_path_uses_the_base_that_holds_it(self, tmp_path: Path) -> None:
        repo = tmp_path / "repo"
        build = tmp_path / "build"
        (repo / "cmake").mkdir(parents=True)
        (repo / "cmake" / "Helpers.cmake").touch()
        assert repo_path("cmake/Helpers.cmake", bases=(build, repo), repo_root=repo) == (
            "cmake/Helpers.cmake"
        )
        assert repo_path("cmake/Other.cmake", bases=(build, repo), repo_root=repo) == ""

    def test_absolute_paths_inside_and_outside_the_checkout(self, tmp_path: Path) -> None:
        repo = tmp_path / "repo"
        inside = repo / "src" / "Vehicle" / "Vehicle.cc"
        assert repo_path(str(inside), bases=(tmp_path,), repo_root=repo) == "src/Vehicle/Vehicle.cc"
        assert (
            repo_path(str(tmp_path / "build" / "a.java"), bases=(tmp_path,), repo_root=repo) == ""
        )
        assert repo_path(str(tmp_path / "repository"), bases=(tmp_path,), repo_root=repo) == ""
        assert repo_path(str(tmp_path), bases=(tmp_path,), repo_root=repo) == ""
