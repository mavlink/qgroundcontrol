#!/bin/bash

die() {
    echo $1
    exit -1
}

cd ~/devel/Firmware || die "Fimrware not installed"

[ -d Tools ] || die "Firmware directory not complete"

# San Jose (Moffett Field)
export PX4_HOME_LAT=37.430181
export PX4_HOME_LON=-122.055202
export PX4_HOME_ALT=5

HEADLESS=1 make px4_sitl_default gazebo
