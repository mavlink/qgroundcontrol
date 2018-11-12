#!/bin/bash

testdir="$(dirname "$(readlink -f $0)")"

rm -f "$testdir/test*"
$top_builddir/shpcreate "$testdir/test" point

$top_builddir/shpadd "$testdir/test" -83.54949956              34.992401
$top_builddir/shpadd "$testdir/test" -83.52162155              34.99276748
$top_builddir/shpadd "$testdir/test" -84.01681518              34.67275985
$top_builddir/shpadd "$testdir/test" -84.15596023              34.64862437
$top_builddir/shpadd "$testdir/test" -83.61951463              34.54927047

$top_builddir/dbfcreate "$testdir/test" -s fd 30
$top_builddir/dbfadd "$testdir/test" "1"
$top_builddir/dbfadd "$testdir/test" "2"
$top_builddir/dbfadd "$testdir/test" "3"
$top_builddir/dbfadd "$testdir/test" "4"
$top_builddir/dbfadd "$testdir/test" "5"

$top_builddir/contrib/shpproj "$testdir/test" "$testdir/test_1" -i=geographic -o="init=nad83:1002 units=us-ft"
$top_builddir/contrib/shpproj "$testdir/test_1" "$testdir/test_2" -o="proj=utm zone=16 units=m"
$top_builddir/contrib/shpproj "$testdir/test_2" "$testdir/test_3" -o=geographic

$top_builddir/shpdump -precision 8 "$testdir/test"    > "$testdir/test.out"
$top_builddir/shpdump -precision 8 "$testdir/test_3"  > "$testdir/test_3.out"


result="$(diff "$testdir/test.out" "$testdir/test_3.out")"
if [ "$result" == "" ]; then
	echo "******* Test Succeeded *********"
	rm -f "$testdir/test*"
	exit 0
else
	echo "******* Test Failed *********"
	echo "$result"
	rm -f "$testdir/test*"
	exit 1
fi
