#!/bin/sh
cp -r ../release/qgroundcontrol.app .


cp -r ../files/audio qgroundcontrol.app/Contents/MacOs/.
mkdir -p qgroundcontrol.app/Contents/Frameworks/
mkdir -p qgroundcontrol.app/Contents/PlugIns/imageformats
mkdir -p qgroundcontrol.app/Contents/PlugIns/codecs
mkdir -p qgroundcontrol.app/Contents/PlugIns/accessible
cp -r /usr/local/Cellar/qt/4.8.5/plugins/ qgroundcontrol.app/Contents/PlugIns/.
# SDL is not copied by Qt - for whatever reason
cp -r /Library/Frameworks/SDL.framework qgroundcontrol.app/Contents/Frameworks/.
echo -e '\n\nStarting to create disk image. This may take a while..\n'
macdeployqt qgroundcontrol.app -dmg
rm -rf qgroundcontrol.app
echo -e '\n\n QGroundControl .DMG file is now ready for publishing\n'
