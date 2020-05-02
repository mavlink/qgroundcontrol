#!/bin/bash
# This is set to find lupdate in my particular installation. You will need to set the path
# where you have Qt installed.
QT_PATH=~//Qt/5.12.6/gcc_64/bin
rm qgc.ts
$QT_PATH/lupdate ../src -ts qgc.ts
python qgc-lupdate-json.py
mv qgc.ts.new qgc.ts
