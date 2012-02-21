#!/bin/bash
QSERIAL_TAG=004e3de552fe25fee593dfcb03e2ffa82cb0b152
MAVLINK_VERSION=1.0.0

libList="mavlink qserialport"

topDir=$PWD


function fetch_git
{
    name=$1
    url=$2
    tag=$4

    echo
    echo updating: $name @ $url to tag $tag
    cd $topDir
	rm -rf $name
	git clone $url
    cd $name && git checkout $tag && rm -rf .git
    cd $topDir
}

function processLib
{
    lib=$1
    case $lib in
		"qserialport")
            fetch_git qserialport git://gitorious.org/inbiza-labs/qserialport.git master $QSERIAL_TAG
			;;
		"mavlink") 
            rm -rf mavlink
            mavlinkName=mavlink-$MAVLINK_VERSION-Linux
            wget https://github.com/downloads/mavlink/mavlink/$mavlinkName.tar.gz
            tar -xzvf $mavlinkName.tar.gz
            mv  $mavlinkName/usr/local/include/mavlink mavlink
            rm -rf $mavlinkName
            rm -rf $mavlinkName.tar.gz
			;;
		"all")
            for lib in $libList
            do
                $0 $lib
            done
			exit 0
			;;
		"exit")
			exit 0
			;;
		*)
			echo unknown lib, possiblities are: $libList
			exit 1
	esac
}


if [ $# == 0 ]
then

    #menu 
    echo This script grabs upstream releases.
    PS3='Please enter your choice: '
    select OPT in $libList all exit
    do
        processLib $OPT
    done
    
elif [ $# == 1 ]
then
    lib=$1
    processLib $lib
else
    echo usage: $0 lib
fi
