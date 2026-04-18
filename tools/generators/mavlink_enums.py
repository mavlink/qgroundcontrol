#!/usr/bin/env python3
"""Extract enum definitions from MAVLink dialect headers and emit:

  1. MAVLinkEnums.h       — C-compatible enum typedefs (unchanged behavior)
  2. MAVLinkEnumsQml.h    — Q_NAMESPACE + Q_ENUM_NS wrapper for QML exposure
  3. MAVLinkEnumsQml.cc   — anchor TU so AUTOMOC generates the meta-object

Usage: mavlink_enums.py <mavlink_include_dir> <enums_h_output> [<qml_h_output> <qml_cc_output>]

If the QML paths are omitted they are derived from <enums_h_output>'s directory.
"""
import os
import re
import sys


ENUM_DECL_RE = re.compile(r'^\s*typedef\s+enum\s+([A-Z_][A-Z0-9_]*)\b', re.MULTILINE)


def extract_enum_block(dialect_header_path):
    try:
        with open(dialect_header_path, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        return ""

    in_enums = False
    out = []
    for line in lines:
        if '// ENUM DEFINITIONS' in line:
            in_enums = True
            continue
        if '// MESSAGE DEFINITIONS' in line:
            break
        if in_enums:
            out.append(line)
    return ''.join(out)


def find_dialects(mavlink_dir):
    dialects = []
    for entry in sorted(os.listdir(mavlink_dir)):
        dialect_dir = os.path.join(mavlink_dir, entry)
        dialect_header = os.path.join(dialect_dir, f"{entry}.h")
        if os.path.isdir(dialect_dir) and os.path.isfile(dialect_header):
            dialects.append((entry, dialect_header))
    return dialects


def write_if_changed(path, content):
    """Return True if file was written (avoids spurious rebuilds)."""
    if os.path.isfile(path):
        with open(path, 'r') as f:
            if f.read() == content:
                return False
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    with open(path, 'w') as f:
        f.write(content)
    return True


def strip_duplicate_blocks(enums_text, seen_names, dialect):
    """Drop duplicate `typedef enum X { ... } X;` blocks; warn on each skip."""
    out = []
    i = 0
    while True:
        m = ENUM_DECL_RE.search(enums_text, i)
        if not m:
            out.append(enums_text[i:])
            break
        out.append(enums_text[i:m.start()])

        name = m.group(1)
        closer = re.compile(r'\}\s*' + re.escape(name) + r'\s*;', re.MULTILINE)
        c = closer.search(enums_text, m.start())
        if not c:
            out.append(enums_text[m.start():])
            break
        block_end = c.end()

        if name in seen_names:
            sys.stderr.write(
                f"mavlink_enums.py: duplicate enum '{name}' in dialect '{dialect}' — "
                "keeping first occurrence\n"
            )
        else:
            seen_names.add(name)
            out.append(enums_text[m.start():block_end])
        i = block_end
    return ''.join(out)


def build_enums_header(dialects):
    parts = ["""\
#pragma once

/// @file MAVLinkEnums.h
/// @brief MAVLink enum definitions only - no message pack/unpack code.
///
/// AUTO-GENERATED from MAVLink dialect headers during the build.
/// Do not edit manually. Regenerate via tools/generators/mavlink_enums.py.

#ifdef __cplusplus
extern "C" {
#endif
"""]
    seen = set()
    for name, path in dialects:
        enums = extract_enum_block(path)
        if not enums.strip():
            continue
        filtered = strip_duplicate_blocks(enums, seen, name)
        if filtered.strip():
            parts.append(f"\n// ---- enums from {name}/{name}.h ----\n")
            parts.append(filtered)
    parts.append("""
#ifdef __cplusplus
}
#endif
""")
    # Preserve deterministic order: names in first-seen dialect order.
    enum_names = []
    seen_for_names = set()
    for line in ''.join(parts).splitlines():
        m = ENUM_DECL_RE.match(line)
        if m and m.group(1) not in seen_for_names:
            seen_for_names.add(m.group(1))
            enum_names.append(m.group(1))
    return ''.join(parts), enum_names


def build_qml_header(enum_names):
    using_lines = '\n'.join(f'    using ::{n};' for n in enum_names)
    q_enum_lines = '\n'.join(f'    Q_ENUM_NS({n})' for n in enum_names)
    return f"""\
#pragma once

/// @file MAVLinkEnumsQml.h
/// @brief Q_NAMESPACE wrapper exposing every MAVLink enum to QML and Qt meta-object.
///
/// AUTO-GENERATED. Do not edit manually.

#include <QtQmlIntegration/QtQmlIntegration>

#include "MAVLinkEnums.h"

namespace MAVLinkEnums {{
    Q_NAMESPACE
    QML_NAMED_ELEMENT(MAVLinkEnums)

{using_lines}

{q_enum_lines}
}}
"""


def build_qml_anchor_cc():
    return """\
/// AUTO-GENERATED. Anchor translation unit so AUTOMOC generates the
/// meta-object for the MAVLinkEnums namespace.
#include "MAVLinkEnumsQml.h"
"""


def main():
    if len(sys.argv) not in (3, 5):
        print(
            f"Usage: {sys.argv[0]} <mavlink_include_dir> <enums_h_output> "
            "[<qml_h_output> <qml_cc_output>]",
            file=sys.stderr,
        )
        sys.exit(1)

    mavlink_dir = sys.argv[1]
    enums_h_path = sys.argv[2]

    if not os.path.isdir(mavlink_dir):
        print(f"Error: {mavlink_dir} is not a directory", file=sys.stderr)
        sys.exit(1)

    dialects = find_dialects(mavlink_dir)
    if not dialects:
        print(f"Error: no dialect headers found in {mavlink_dir}", file=sys.stderr)
        sys.exit(1)

    enums_h, enum_names = build_enums_header(dialects)
    if len(sys.argv) == 5:
        qml_h_path = sys.argv[3]
        qml_cc_path = sys.argv[4]
    else:
        out_dir = os.path.dirname(os.path.abspath(enums_h_path))
        qml_h_path = os.path.join(out_dir, "MAVLinkEnumsQml.h")
        qml_cc_path = os.path.join(out_dir, "MAVLinkEnumsQml.cc")

    written = []
    if write_if_changed(enums_h_path, enums_h):          written.append(enums_h_path)
    if write_if_changed(qml_h_path, build_qml_header(enum_names)): written.append(qml_h_path)
    if write_if_changed(qml_cc_path, build_qml_anchor_cc()):       written.append(qml_cc_path)

    if written:
        print(f"Generated {len(enum_names)} enums from {len(dialects)} dialects -> {', '.join(os.path.basename(p) for p in written)}")
    else:
        print(f"MAVLinkEnums.h / MAVLinkEnumsQml.{{h,cc}} up to date")


if __name__ == '__main__':
    main()
