#!/bin/sh
cp -r ../../qgroundcontrol-build-desktop/qgroundcontrol.app .


cp -r ../audio qgroundcontrol.app/Contents/MacOs/.
mkdir -p qgroundcontrol.app/Contents/Frameworks/
# SDL is not copied by Qt - for whatever reason
cp -r /Library/Frameworks/SDL.framework qgroundcontrol.app/Contents/Frameworks/.
echo -e '\n\nStarting to create disk image. This may take a while..\n'
macdeployqt qgroundcontrol.app -dmg
rm -rf qgroundcontrol.app
echo -e '\n\n QGroundControl .DMG file is now ready for publishing\n'
