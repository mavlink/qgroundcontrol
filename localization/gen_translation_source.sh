#!/bin/bash
# This is set to find lupdate in my particular installation. You will need to set the path
# where you have Qt installed.
QT_PATH=~/Applications/Qt/5.9.1/clang_64/bin
$QT_PATH/lupdate ../src -ts qgc.ts
