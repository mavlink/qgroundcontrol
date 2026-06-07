#!/usr/bin/env python3
# Copyright (C) 2017 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import print_function

import argparse
import json
import subprocess
from distutils.version import StrictVersion

def is_available(object):
    if "isAvailable" in object:
        return object["isAvailable"]   # introduced in Xcode 11
    else:
        return "unavailable" not in object["availability"]

def is_suitable_runtime(runtimes, runtime_name, platform, min_version):
    for runtime in runtimes:
        identifier = runtime["identifier"]
        if (runtime["name"] == runtime_name or identifier == runtime_name) \
            and is_available(runtime) \
            and identifier.startswith("com.apple.CoreSimulator.SimRuntime.{}".format(platform)) \
            and StrictVersion(runtime["version"]) >= min_version:
            return True
    return False

def simctl_runtimes():
    return json.loads(subprocess.check_output(
        ["/usr/bin/xcrun", "simctl", "list", "runtimes", "--json"]))["runtimes"]

def simctl_devices():
    return json.loads(subprocess.check_output(
        ["/usr/bin/xcrun", "simctl", "list", "devices", "--json"]))["devices"]

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--platform', choices=['iOS', 'tvOS', 'watchOS'], required=True)
    parser.add_argument('--minimum-deployment-target', type=StrictVersion, default='0.0')
    parser.add_argument('--state',
        choices=['booted', 'shutdown', 'creating', 'booting', 'shutting-down'], action='append')
    args = parser.parse_args()

    runtimes = simctl_runtimes()
    device_dict = simctl_devices()
    for runtime_name in device_dict:
        if is_suitable_runtime(runtimes, runtime_name, args.platform, args.minimum_deployment_target):
            for device in device_dict[runtime_name]:
                if is_available(device) \
                    and (args.state is None or device["state"].lower() in args.state):
                    print(device["udid"])
