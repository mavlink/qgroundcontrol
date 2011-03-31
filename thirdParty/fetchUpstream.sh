#!/bin/bash

PS3='Please enter your choice: '
LIST="all mavlink qserialport end"
echo 
echo this script grabs upstream releases
echo

function fetch_qserialport
{
	echo
	rm -rf qserialport
	git clone git://gitorious.org/inbiza-labs/qserialport.git
	rm -rf qserialport/.git
}

function fetch_mavlink
{
	echo
	rm -rf mavlink
	git clone git@github.com:openmav/mavlink.git
	rm -rf mavlink/.git
}

echo
select OPT in $LIST
do
	case $OPT in
		"qserialport")
			fetch_qserialport
			exit 0
			;;
		"mavlink") 
			fetch_mavlink
			exit 0
			;;
		"all")
			fetch_mavlink
			fetch_qserialport
			exit 0
			;;
		"exit")
			exit 0
			;;
		*)
			echo unknown option
			exit 1
	esac
done


