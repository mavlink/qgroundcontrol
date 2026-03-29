#!/usr/bin/env python3
"""Extract enum definitions from MAVLink dialect headers into a standalone header.

Usage: python3 GenerateMAVLinkEnums.py <mavlink_include_dir> <output_file>

Example:
    python3 GenerateMAVLinkEnums.py build/_deps/mavlink-build/include/mavlink build/MAVLinkEnums.h
"""
import os
import sys

def extract_enums(dialect_header_path):
    """Extract text between '// ENUM DEFINITIONS' and '// MESSAGE DEFINITIONS' markers."""
    try:
        with open(dialect_header_path, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        return ""

    in_enums = False
    enum_lines = []
    for line in lines:
        if '// ENUM DEFINITIONS' in line:
            in_enums = True
            continue
        if '// MESSAGE DEFINITIONS' in line:
            break
        if in_enums:
            enum_lines.append(line)
    return ''.join(enum_lines)


def find_dialects(mavlink_dir):
    """Find all dialect directories that contain a dialect header."""
    dialects = []
    for entry in sorted(os.listdir(mavlink_dir)):
        dialect_dir = os.path.join(mavlink_dir, entry)
        dialect_header = os.path.join(dialect_dir, f"{entry}.h")
        if os.path.isdir(dialect_dir) and os.path.isfile(dialect_header):
            dialects.append((entry, dialect_header))
    return dialects


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <mavlink_include_dir> <output_file>", file=sys.stderr)
        sys.exit(1)

    mavlink_dir = sys.argv[1]
    output_file = sys.argv[2]

    if not os.path.isdir(mavlink_dir):
        print(f"Error: {mavlink_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    dialects = find_dialects(mavlink_dir)
    if not dialects:
        print(f"Error: no dialect headers found in {mavlink_dir}", file=sys.stderr)
        sys.exit(1)

    # Build output
    parts = []
    parts.append("""\
#pragma once

/// @file MAVLinkEnums.h
/// @brief MAVLink enum definitions only - no message pack/unpack code.
///
/// AUTO-GENERATED from MAVLink dialect headers during the build.
/// Do not edit manually. Regenerate via cmake/GenerateMAVLinkEnums.py.
///
/// This header provides all MAVLink enum typedefs (~3,700 lines) without
/// the ~177,000 lines of message pack/encode/decode functions.
/// Use this in headers that need MAVLink enum types (MAV_CMD, MAV_TYPE, etc.)
/// but don't need message struct definitions.

#ifdef __cplusplus
extern "C" {
#endif
""")

    for name, path in dialects:
        enums = extract_enums(path)
        if enums.strip():
            parts.append(f"\n// ---- enums from {name}/{name}.h ----\n")
            parts.append(enums)

    parts.append("""
#ifdef __cplusplus
}
#endif
""")

    output = ''.join(parts)

    # Ensure output directory exists
    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)

    # Only write if content changed (avoid unnecessary rebuilds)
    if os.path.isfile(output_file):
        with open(output_file, 'r') as f:
            if f.read() == output:
                print(f"MAVLinkEnums.h is up to date ({output_file})")
                return

    with open(output_file, 'w') as f:
        f.write(output)

    # Count enums
    enum_count = output.count('typedef enum')
    line_count = output.count('\n')
    print(f"Generated {output_file}: {line_count} lines, {enum_count} enums from {len(dialects)} dialects")


if __name__ == '__main__':
    main()
