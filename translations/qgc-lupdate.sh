#!/bin/bash
# This script will update both the Qt and Json string translation files.
QT_PATH=~/Qt/6.6.*/gcc_64/bin
$QT_PATH/lupdate ../src -ts qgc.ts -no-obsolete
python3 qgc-lupdate-json.py
