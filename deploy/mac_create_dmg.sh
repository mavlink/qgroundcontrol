#!/bin/sh
# Clean build directories
rm -rf mac
mkdir -p mac
# Change to build directory and compile application
cd ..
make -j4
# Copy and build the application bundle
cd deploy
cp -r ../bin/mac/qgroundcontrol.app mac/.


cp -r ../audio mac/qgroundcontrol.app/Contents/MacOs/.
mkdir -p mac/qgroundcontrol.app/Contents/Frameworks/
# SDL is not copied by Qt - for whatever reason
cp -r SDL.framework mac/qgroundcontrol.app/Contents/Frameworks/.
echo -e '\n\nStarting to create disk image. This may take a while..\n'
macdeployqt mac/qgroundcontrol.app -dmg
rm -rf mac/qgroundcontrol.app
echo -e '\n\n QGroundControl .DMG file is now ready for publishing\n'
