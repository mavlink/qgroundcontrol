#!/usr/bin/env python3
"""Build the activation-only wire codec and verify its STATUS digest fixture."""

import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    vector = json.loads((HERE / "status_digest_vector.json").read_text(encoding="utf-8"))
    actual = hashlib.blake2b(bytes.fromhex(vector["request_hex"]), digest_size=32).hexdigest()
    if actual != vector["blake2b_256"]:
        raise RuntimeError("STATUS digest does not match activation request fixture")
    with tempfile.TemporaryDirectory(prefix="amov-activation-codec-") as temporary:
        executable = str(Path(temporary) / "test_codec")
        subprocess.run(
            [
                args.cxx,
                "-std=c++11",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-pedantic",
                str(HERE / "test_codec.cpp"),
                "-o",
                executable,
            ],
            check=True,
        )
        subprocess.run([executable], check=True)


if __name__ == "__main__":
    main()
