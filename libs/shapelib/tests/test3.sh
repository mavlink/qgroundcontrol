#!/bin/bash

#
#	Use example programs to create a very simple dataset that
#	should display in ARCView II.
#

testdir="$(dirname "$(readlink -f $0)")"

(
cd "$top_builddir"
./shpcreate test polygon
./dbfcreate test.dbf -s Description 30 -n TestInt 6 0 -n TestDouble 16 5

./shpadd test 0 0 100 0 100 100 0 100 0 0 + 20 20 20 30 30 30 20 20
./dbfadd test.dbf "Square with triangle missing" 1.4 2.5

./shpadd test 150 150 160 150 180 170 150 150
./dbfadd test.dbf "Smaller triangle" 100 1000.25

./shpadd test 150 150 160 150 180 170 150 150
./dbfadd test.dbf "" "" ""

./shpdump test.shp
./dbfdump test.dbf
) > "$testdir/s3.out"

result=$(diff "$testdir/s3.out" "$testdir/stream3.out")
if [ "$result" == "" ]; then
	echo "******* Stream 3 Succeeded *********"
	rm "$testdir/s3.out"
	exit 0
else
	echo "******* Stream 3 Failed *********"
	echo "$result"
	rm "$testdir/s3.out"
	exit 1
fi
