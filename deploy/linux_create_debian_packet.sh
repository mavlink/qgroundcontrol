#!/bin/sh
# Clean build directories
rm -rf linux
mkdir -p linux
# Change to build directory and compile application
cd ..
make -j4
# Copy and build the application bundle
cd deploy
cp -r qgroundcontrol linux/.
cp -r ../audio linux/.
# FIXME Create debian packet
echo -e '\n QGroundControl Debian packet is now ready for publishing\n'
