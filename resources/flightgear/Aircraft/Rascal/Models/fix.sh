#! /bin/sh

for f in "$@" ; do
  sed -i.before-color-change 's,\(MATERIAL.*\)rgb\(.*\)amb\(.*\)emis\(.*\)spec\(.*\)shi\(.*\)trans\(.*\)$,\1rgb\2amb\2emis\4spec\5shi\6trans\7,1' "$f"
  if ! cmp "${f}" "${f}.before-color-change" > /dev/null 2>&1 ; then
      echo "$f has changed colors!"
  fi
done
