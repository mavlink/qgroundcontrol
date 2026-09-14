"""Read literal MAVLink defaults without depending on CMake line wrapping."""

import argparse
import re
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("config", type=Path)
    parser.add_argument("key", choices=("GIT_REPO", "GIT_TAG", "DIALECT", "VERSION"))
    args = parser.parse_args()
    try:
        print(read_default(args.config, args.key))
    except ValueError as error:
        parser.error(str(error))


def read_default(config: Path, key: str) -> str:
    name = f"QGC_MAVLINK_{key}"
    contents = config.read_text(encoding="utf-8")
    values = re.findall(rf'^\s*set\(\s*{name}\s+"([^"\r\n]+)"', contents, re.MULTILINE)
    if len(values) != 1:
        raise ValueError(f"Expected one nonempty quoted default for {name} in {config}")
    return values[0]


if __name__ == "__main__":
    main()
