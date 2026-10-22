"""Entry point for the CMake-generated moccache launcher.

Python never caches bytecode for the script it runs, so the launcher runs this
stub and imports moccache, whose bytecode is cached in tools/__pycache__. That
saves recompiling moccache.py on every moc run (hundreds per clean build).
When tools/ is read-only, Python skips writing the cache and still runs it.
"""

import sys

from moccache import main

if __name__ == "__main__":
    sys.exit(main())
