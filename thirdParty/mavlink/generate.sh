#!/bin/bash
# this script generates all the include files with pymavlink

# settings
wireProtocol=1.0
pymavlinkTag=51f3d6713e9a5b94c232ab9bf9d08095a0c97866

# download pymavlink
topDir=$PWD
rm -rf include
rm -rf pymavlink
git clone https://github.com/mavlink/pymavlink.git -b master  pymavlink
cd pymavlink && git checkout $pymavlinkTag && rm -rf .git

# generate includes using message definitions
cd $topDir
for file in $(find message_definitions -name "*.xml")
do
    echo generating mavlink includes for definition: $file
    ./pymavlink/generator/mavgen.py --lang=C --wire-protocol=$wireProtocol --output=include $file
done

# cleanup
rm -rf pymavlink
