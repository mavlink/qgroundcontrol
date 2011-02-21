#!/bin/bash

PS3='Please enter your choice: '
LIST="in_source_build install_build grab_debian_dependencies package_source package remake clean END"
MAKEARGS="-j8"
echo 
echo in_source_build: is used for development and you can start the scicoslab toolbox by typing scicoslab in the oooark source directory
echo install_build: is used for building before final installation to the system.
echo grab_debian_dependencies: installs all the required packages for debian based systems \(ubuntu maverick/ debian squeeze,lenny\)
echo remake: calls make again after project has been configured as install or in source build
echo package_source: creates a source package for distribution
echo package: creates binary packages for distribution
echo clean: removes the build directory 

echo
select OPT in $LIST
do
	if [ $OPT = "in_source_build" ] &> /dev/null
	then
		echo you chose in source build
		mkdir -p build && cd build && cmake -DIN_SRC_BUILD:bool=TRUE .. && make $MAKEARGS
		exit 0
	elif [ $OPT = "install_build" ] &> /dev/null
	then
		echo you chose install build
		mkdir -p build && cd build && cmake .. && make $MAKEARGS
		exit 0
	elif [ $OPT = "grab_debian_dependencies" ] &> /dev/null
	then
		echo you chose to install debian dependencies
		sudo apt-get install cmake libqt4-dev libboost-all-dev libopenscenegraph-dev
		sudo apt-get install scicoslab-gtk
		exit 0

	elif [ $OPT = "remake" ] &> /dev/null
	then
		echo you chose to recall make on the previously configured build
		cd build && make $MAKEARGS
		exit 0

	elif [ $OPT = "package_source" ] &> /dev/null
	then
		echo you chose to package the source
		mkdir -p build && cd build && cmake .. && make package_source
		exit 0

	elif [ $OPT = "package" ] &> /dev/null
	then
		echo you chose to package the binary
		mkdir -p build && cd build && cmake .. && make package
		exit 0

	elif [ $OPT = "clean" ] &> /dev/null
	then
		echo you chose to clean the build 
		rm -rf build

	elif [ $OPT = "END" ] &> /dev/null
	then
		exit 0
	fi
done
