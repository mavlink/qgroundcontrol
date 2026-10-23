"""Entry point for the CMake-generated moccache launcher and the moccache CLI.

Python never caches bytecode for the script it runs, so the launcher runs this
stub and imports the moccache package, whose bytecode is cached in
tools/moccache/__pycache__. That saves recompiling it on every moc run
(hundreds per clean build). When tools/ is read-only, Python skips writing the
cache and still runs it.
"""

import sys

from moccache.core import main

if __name__ == "__main__":
    sys.exit(main())
