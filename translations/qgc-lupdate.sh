#!/bin/bash
# This script will update both the Qt and Json string translation files.
QT_PATH=~//Qt/5.12.6/gcc_64/bin
rm qgc-qt.ts
$QT_PATH/lupdate ../src -ts qgc.ts
python qgc-lupdate-json.py
