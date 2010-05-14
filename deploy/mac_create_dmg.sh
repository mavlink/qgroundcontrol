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
macdeployqt qgroundcontrol.app --bundle
echo -e '\n QGroundControl .DMG file is now ready for publishing\n'
