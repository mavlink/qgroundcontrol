#!/usr/bin/env python3

import os
import subprocess
from pathlib import Path

BUILD_FOLDER=os.getenv("BUILD_FOLDER")
CONTENT_FOLDER=f"{BUILD_FOLDER}/staging/QGroundControl.app/Contents/"
BASE_PATH="@executable_path/../"

if not BUILD_FOLDER or not os.path.exists(BUILD_FOLDER):
    print(f"BUILD_FOLDER does not exist or was not set: {BUILD_FOLDER}")

if not os.path.exists(CONTENT_FOLDER):
    print(f"Content folder does not exist: {CONTENT_FOLDER}")

def find_files(file_name: str):
    return [str(file) for file in Path(CONTENT_FOLDER).rglob(file_name)]

def get_rpaths(file: str):
    process = subprocess.Popen(['otool', '-L', file], stdout=subprocess.PIPE)
    paths = [line.decode('unicode_escape').strip().split(" ")[0] for line in process.stdout]
    rpaths = filter(lambda file: "@rpath" in file, paths)
    return list(rpaths)

def update_file_rpath(file: str, old_rpath: str, new_rpath: str):
    print(f"{file} => {old_rpath} -> {new_rpath}")
    command = f"install_name_tool -change {old_rpath} {new_rpath} {file}".split(" ")
    subprocess.Popen(command, stdout=subprocess.PIPE)

#get_rpaths(f"{CONTENT_FOLDER}/Frameworks/GStreamer.framework/Versions/1.0/lib/libgstnet-1.0.dylib")

for file in find_files("*.dylib"):
    for rpath in get_rpaths(file):
        rpath_file = rpath[len("@rpath/"):]
        possible_rpath_candidates = find_files(rpath_file)
        if not possible_rpath_candidates:
            print(f"Failed to find candidate for: {file} -> {rpath_file}")
            exit(1)

        # Remove path that already exist in original rpath reference
        new_rpath = BASE_PATH + possible_rpath_candidates[0][len(CONTENT_FOLDER):] #[:-(len(rpath_file) + 1)]
        update_file_rpath(file, rpath, new_rpath)
