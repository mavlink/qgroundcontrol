#!/bin/sh
cp -r ../../build-qgroundcontrol-Qt_4_8_1-Release/release/qgroundcontrol.app .


cp -r ../files/audio qgroundcontrol.app/Contents/MacOs/.
mkdir -p qgroundcontrol.app/Contents/Frameworks/
mkdir -p qgroundcontrol.app/Contents/PlugIns/imageformats
mkdir -p qgroundcontrol.app/Contents/PlugIns/codecs
mkdir -p qgroundcontrol.app/Contents/PlugIns/accessible
# SDL is not copied by Qt - for whatever reason
cp -r /Library/Frameworks/SDL.framework qgroundcontrol.app/Contents/Frameworks/.
echo -e '\n\nStarting to create disk image. This may take a while..\n'
$HOME/QtSDK/Desktop/Qt/4.8.1/gcc/bin/macdeployqt qgroundcontrol.app -dmg
rm -rf qgroundcontrol.app
echo -e '\n\n QGroundControl .DMG file is now ready for publishing\n'
