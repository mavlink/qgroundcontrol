#!/bin/bash

testdir="$(dirname "$(readlink -f $0)")"

(
cd "$top_builddir"
for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13; do
  echo -----------------------------------------------------------------------
  echo Test 2/$i
  echo -----------------------------------------------------------------------

  ./shptest $i
  ./shpdump test${i}.shp
done
) > "$testdir/s2.out"

result="$(diff "$testdir/s2.out" "$testdir/stream2.out")"
if [ "$result" == "" ]; then
	echo "******* Stream 2 Succeeded *********"
	rm "$testdir/s2.out"
	exit 0
else
	echo "******* Stream 2 Failed *********"
	echo "$result"
	rm "$testdir/s2.out"
	exit 1
fi
