#!/usr/bin/env python3
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import subprocess
import sys
import base64
import time
import signal
import argparse
import re
import xml.etree.ElementTree as ET

def status(msg):
    print(f"\n-- {msg}")

def error(msg):
    print(f"Error: {msg}", file=sys.stderr)

def die(msg):
    error(msg)
    sys.exit(1)

# Define and parse arguments
parser = argparse.ArgumentParser(description="Qt for Android app runner.",
                                 epilog=f'''
This is a helper script to run Qt for Android apps directly from the terminal.
It supports starting apps with parameters and forwards environment variables to
the device. It prints live logcat messages as the app is running. The script exits
once the app has exited on the device and terminates the app on the device if the
script is terminated.

If an APK path is provided, it will first be installed to the device only if the
--install parameter is passed.

Use --serial parameter or adb's ANDROID_SERIAL environment variable to specify an
Android target serial number (obtained from "adb devices" command) on which to run
the app or test.
''', formatter_class=argparse.RawTextHelpFormatter)

parser.add_argument('-a', '--adb', metavar='path', type=str, help='Path to adb executable.')
parser.add_argument('-b', '--build-path', metavar='path', type=str,
                    help='Path to the Android build directory.')
parser.add_argument('-i', '--install', action='store_true', help='Install the APK.')
parser.add_argument('-d', '--detached', action='store_true',
                    help='Start the app detached without waiting for the logcat')
parser.add_argument('-s', '--serial', type=str, metavar='serial',
                    help='Android device serial (override $ANDROID_SERIAL).')
parser.add_argument('-p', '--apk', type=str, metavar='path', help='Path to the APK file.')

args, remaining_args = parser.parse_known_args()

# Validate required arguments
if not args.build_path:
    die("App build path is not provided")

adb = args.adb
if not adb:
    adb = 'adb'
    null_dev = subprocess.DEVNULL
    if subprocess.call(['command', '-v', adb], stdout=null_dev, stderr=null_dev) != 0:
        die("adb tool path is not provided and is not found in PATH")

try:
    devices = []
    output = subprocess.check_output(f"{adb} devices", shell=True).decode().strip()
    for line in output.splitlines():
        if '\tdevice' in line:
            serial = line.split('\t')[0]
            devices.append(serial)
    if not devices:
        die("No devices are connected.")

    if args.serial and not args.serial in devices:
        die("No connected devices with the specified serial number.")
except Exception as e:
    die(f"Failed to check for running devices, received error: {e}")

if args.serial:
    adb = f"{adb} -s {args.serial}"

if args.build_path is None:
    die("App build path is not provided")

if args.apk and args.install:
    status(f"Installing the app APK {args.apk}")
    try:
        subprocess.run(f"{adb} install \"{args.apk}\"", check=True, shell=True)
    except Exception as e:
        error(f"Failed to install the APK, received error: {e}")


def find_launcher_activity(root):
    ns_android = 'http://schemas.android.com/apk/res/android'
    android_name_attr = f'{{{ns_android}}}name'

    for activity in root.findall('.//activity'):
        for intent_filter in activity.findall('intent-filter'):
            actions = {action.get(android_name_attr) for action in intent_filter.findall('action')}
            categories = {cat.get(android_name_attr) for cat in intent_filter.findall('category')}
            main_action = 'android.intent.action.MAIN'
            launcher_category = 'android.intent.category.LAUNCHER'
            if main_action in actions and launcher_category in categories:
                return activity.get(android_name_attr)
    return None

def get_manifest_app_details(manifest_file):
    try:
        if not os.path.isfile(manifest_file):
            return None, None

        tree = ET.parse(manifest_file)
        root = tree.getroot()
        package_name = root.get("package")
        activity_name = find_launcher_activity(root)

        if activity_name and activity_name.startswith('.') and package_name:
            activity_name = package_name + activity_name

        return package_name, activity_name
    except Exception as e:
        error(f"Failed to parse AndroidManifest.xml, received error: {e}")
        return None, None

def get_package_from_gradle(build_path):
    try:
        # Check build.gradle for namespace
        gradle_file = os.path.join(build_path, "build.gradle")
        if os.path.isfile(gradle_file):
            with open(gradle_file) as f:
                for line in f:
                    if line.strip().startswith("namespace"):
                        # Match the following cases:
                        #   namespace "org.qtproject.example.app"
                        #   namespace 'org.qtproject.example.app'
                        #   namespace = "org.qtproject.example.app"
                        #   namespace = 'org.qtproject.example.app'
                        match = re.search(r"namespace\s*=?\s*['\"]([^'\"]+)['\"]", line)
                        if match:
                            potential_package_name = match.group(1)
                            if potential_package_name != "androidPackageName":
                                return potential_package_name

        # Check gradle.properties for androidPackageName
        properties_file = os.path.join(build_path, "gradle.properties")
        if os.path.isfile(properties_file):
            with open(properties_file) as f:
                for line in f:
                    if line.startswith("androidPackageName="):
                        return line.split('=')[1].strip()
    except Exception as e:
        error(f"Failed to retrieve the app's package name, received error: {e}")

    return None

def get_app_details(build_path):
    manifest_file = os.path.join(build_path, "AndroidManifest.xml")
    package_name, activity_name = get_manifest_app_details(manifest_file)

    if not package_name:
        package_name = get_package_from_gradle(build_path)

    return package_name, activity_name


# Get app details
package_name, activity_name = get_app_details(args.build_path)
if not package_name:
    die("Failed to retrieve the package name of the app")
if not activity_name:
    die("Failed to retrieve the main activity name of the app")

start_cmd = f"{adb} shell am start -n {package_name}/{activity_name}"

# Get environment variables
env_vars = " ".join(f"{key}={value}" for key, value in os.environ.items())
encoded_env_vars = base64.b64encode(env_vars.encode()).decode()
start_cmd += f" -e extraenvvars \"{encoded_env_vars}\""

# Get app arguments
if remaining_args:
    start_cmd += f" -e applicationArguments \"{' '.join(remaining_args)}\""

# Get formatted time from device
start_timestamp = ""
try:
    start_timestamp = subprocess.check_output(f"{adb} shell \"date +'%Y-%m-%d %H:%M:%S.%3N'\"",
                                              shell=True).decode().strip()
except Exception as e:
    die(f"Failed to get formatted time from the device, received error: {e}")

try:
    subprocess.run(start_cmd, check=True, shell=True)
except Exception as e:
    die(f"Failed to start the app {package_name}, received error: {e}")

# Wait for the app to start and retrieve its pid
start_timeout = 5
time_limit = time.time() + start_timeout
pid = None
while pid is None:
    if time.time() > time_limit:
        die(f"Couldn't retrieve the app's PID within {start_timeout} seconds")
    time.sleep(0.5)
    try:
        pidof_output = subprocess.check_output(f"{adb} shell pidof {package_name}", shell=True)
        pid = pidof_output.decode().strip().split()[0]
    except subprocess.CalledProcessError:
        continue

if args.detached:
    sys.exit(0)

# Add a signal handler to stop the app if the script is terminated
interrupted = False
def terminate_app(signum, frame):
    global interrupted
    interrupted = True

signal.signal(signal.SIGINT, terminate_app)

# Show app's logs
logcat_process = None;
try:
    format_arg = "-v brief -v color"
    time_arg = f"-T '{start_timestamp}'"
    # escape char and color followed with fatal tag
    fatal_regex = "-e $'^\x1b\\[[0-9]*mF/'"
    pid_regex = f"-e '([ ]*{pid}):'"
    logcat_cmd = f"{adb} shell \"logcat {time_arg} {format_arg} | grep {pid_regex} {fatal_regex}\""
    logcat_process = subprocess.Popen(logcat_cmd, shell=True)
except Exception as e:
    die(f"Failed to get logcat for the app {package_name}, received error: {e}")

# Monitor the app's pid
try:
    while not interrupted:
        time.sleep(1)
        try:
            pidof_output = subprocess.check_output(f"{adb} shell pidof {package_name}", shell=True)
            pid = pidof_output.decode().strip()
            if not pid:
                status(f"The app \"{package_name}\" has exited")
                break
        except subprocess.CalledProcessError:
            status(f"The app \"{package_name}\" has exited")
            break
finally:
    if logcat_process:
        logcat_process.terminate()

if interrupted:
    try:
        subprocess.Popen(f"{adb} shell am force-stop {package_name}", shell=True)
        status(f"The app \"{package_name}\" with {pid} has been terminated")
    except Exception as e:
        error(f"Failed to terminate the app {package_name}, received error: {e}")
