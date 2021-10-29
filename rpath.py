#!/usr/bin/env python3

import os
import subprocess
from functools import cache
from pathlib import Path

CONTENT_FOLDER=os.getenv("CONTENT_FOLDER")
BASE_PATH="@executable_path/../"

if not os.path.exists(CONTENT_FOLDER):
    print(f"Content folder does not exist: {CONTENT_FOLDER}")

def find_files(file_name: str):
    return [str(file) for file in Path(CONTENT_FOLDER).rglob(file_name)]

@cache
def get_paths(file: str):
    process = subprocess.Popen(['otool', '-L', file], stdout=subprocess.PIPE)
    paths = [line.decode('unicode_escape').strip().split(" ")[0] for line in process.stdout]
    return paths

def update_file_rpath(file: str, old_rpath: str, new_rpath: str):
    print(f"{file} => {old_rpath} -> {new_rpath}")
    command = f"install_name_tool -change {old_rpath} {new_rpath} {file}".split(" ")
    #subprocess.Popen(command, stdout=subprocess.PIPE)

#get_rpaths(f"{CONTENT_FOLDER}/Frameworks/GStreamer.framework/Versions/1.0/lib/libgstnet-1.0.dylib")

prefixes = ["@rpath/", "/Library/Frameworks/"]

for file in find_files("*.dylib"):
    paths = get_paths(file)
    paths = filter(lambda file: any([prefix in file for prefix in prefixes]), paths)
    scan_paths = map(lambda file: if prefix in file return (prefix, file) else continue for prefix in prefixes, paths)
    for prefix, path in list(scan_paths):
        path_file = path[len(prefix):]
        possible_path_candidates = find_files(path_file)
        if not possible_path_candidates:
            print(f"Failed to find candidate for: {file} -> {path_file}")
            exit(1)

        # Remove path that already exist in original rpath reference
        new_rpath = BASE_PATH + possible_path_candidates[0][(len(str(Path(CONTENT_FOLDER))) + 1):]
        update_file_rpath(file, path, new_path)
