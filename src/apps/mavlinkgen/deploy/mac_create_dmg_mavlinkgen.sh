#!/bin/sh
cp -r ../../mavlinkgen-build-desktop/mavlinkgen.app .

echo -e '\n\nStarting to create disk image. This may take a while..\n'
macdeployqt mavlinkgen.app -dmg
rm -rf mavlinkgen.app
echo -e '\n\n MAVLinkGen .DMG file is now ready for publishing\n'
