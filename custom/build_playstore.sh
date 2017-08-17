#!/bin/bash

# Build locally for a Play Store, signed distribution binary

die() {
    echo $1
    exit -1    
}

[ -z "$KEYSTORE_FILE" ] && die "Env var KEYSTORE_FILE must point to the keystore file"
[ -z "$KEYSTORE_PWD" ]  && die "Env var KEYSTORE_PWD must contain the keystore password"
[ -z "$KEYSTORE_USER" ] && die "Env var KEYSTORE_USER must contain the keystore username"
[ -z "$KEY_PWD" ]  && die "Env var KEY_PWD must contain the key password"

export DPBUILDTYPE=PlayStoreBuild
case $0 in /*) SCRIPT="$0";; *) SCRIPT="`pwd`/${0#./}";; esac
ROOTDIR=`dirname "$SCRIPT"`
$ROOTDIR/build.sh $@
