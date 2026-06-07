#!/usr/bin/env python3
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import re
import sys
import logging as L
from subprocess import Popen, PIPE


# Thin testrunner that ignores failures in tests and only catches
# crashes or ASAN errors.
#
# It executes its arguments as a command line, and parses the stderr for the
# following regex:
detect_ASAN = re.compile(r"^==[0-9]+==ERROR: AddressSanitizer")


my_name = os.path.basename(sys.argv[0])
logging_format = my_name + " %(levelname)8s: %(message)s"
L.basicConfig(format=logging_format, level=L.DEBUG)

L.info("This test is wrapped with sanitizer-testrunner.py. FAIL results are being ignored! Only crashes and ASAN errors are caught.")

proc = None
if sys.argv[1] == "-f":            # hidden option to parse pre-existing files
    f = open(sys.argv[2], "r", errors="ignore")
else:
    proc = Popen(sys.argv[1:], stderr=PIPE, universal_newlines=True, errors="ignore")
    f = proc.stderr

issues_detected = False
for line in f:
    if proc:
        # We don't want the stderr of the subprocess to disappear, so print it.
        print(line, file=sys.stderr, end="", flush=True)
    if detect_ASAN.match(line):
        issues_detected = True
f.close()
if proc:
    proc.wait()
    rc = proc.returncode
    L.info("Test exit code was: %d", rc)
    if not ( 0 <= rc <= 127 ):
        L.error("Crash detected")
        exit(1)

if issues_detected:
    L.error("ASAN issues detected")
    exit(1)
