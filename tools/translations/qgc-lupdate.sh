#!/bin/bash
# This script will update both the Qt and Json string translation files.
# Run from repository root: source tools/translations/qgc-lupdate.sh
QT_PATH=(~/Qt/6.1[0-9].*/*/bin)
"${QT_PATH[0]}"/lupdate src -ts translations/qgc.ts -no-obsolete
python3 tools/translations/qgc-lupdate-json.py
