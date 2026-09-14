#!/usr/bin/env python3
"""Build the MAVLink fuzzer and seed corpus in the ClusterFuzzLite environment."""

from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import sys
import zipfile
from pathlib import Path

from read_mavlink_config import read_default


def build() -> None:
    source, work, output = (Path(os.environ[key]) for key in ("SRC", "WORK", "OUT"))
    config = source / "qgroundcontrol/cmake/CustomOptions.cmake"
    dialect = read_default(config, "DIALECT")
    version = read_default(config, "VERSION")
    generated = work / "mavlink-include/mavlink"
    fuzzer = source / "qgroundcontrol/test/Fuzz/MAVLinkParserFuzzer.cc"
    name = "mavlink_parser_fuzzer"
    subprocess.run(
        [
            sys.executable,
            "-S",
            "-m",
            "pymavlink.tools.mavgen",
            "--no-validate",
            "--lang=C",
            f"--wire-protocol={version}",
            "--output",
            str(generated),
            str(source / f"mavlink/message_definitions/v1.0/{dialect}.xml"),
        ],
        check=True,
        env=os.environ | {"PYTHONPATH": str(source / "mavlink")},
    )
    compiler = [
        os.environ["CXX"],
        *shlex.split(os.environ["CXXFLAGS"]),
        "-std=c++20",
        f"-I{generated}",
        f"-I{generated / dialect}",
    ]
    subprocess.run(
        [
            *compiler,
            str(fuzzer),
            *shlex.split(os.environ["LIB_FUZZING_ENGINE"]),
            "-o",
            str(output / name),
        ],
        check=True,
    )
    generator = work / "mavlink_seed_generator"
    subprocess.run(
        [*compiler, "-DQGC_MAVLINK_SEED_GENERATOR", str(fuzzer), "-o", str(generator)], check=True
    )
    corpus = work / "mavlink-corpus"
    corpus.mkdir(parents=True, exist_ok=True)
    seed = corpus / "heartbeat"
    subprocess.run([str(generator), str(seed)], check=True)
    with zipfile.ZipFile(
        output / f"{name}_seed_corpus.zip", "w", compression=zipfile.ZIP_DEFLATED
    ) as archive:
        archive.write(seed, arcname=seed.name)
    shutil.copyfile(fuzzer.with_suffix(".dict"), output / f"{name}.dict")


def main() -> int:
    try:
        build()
        return 0
    except (OSError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        print(f"Fuzzer build failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
