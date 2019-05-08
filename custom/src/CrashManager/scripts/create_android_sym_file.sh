#!/bin/bash

crashInfoDir="$1"
apkFile=$crashInfoDir/crash.apk
dumpFile=$crashInfoDir/crash.dmp
stackFile=$crashInfoDir/stackWalk.txt
stackWalkLog=$crashInfoDir/stackWalk.log
apkExplodeDir=$crashInfoDir/crash.apk.explode
symbolsDir=$apkExplodeDir/symbols
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
toolsDir=$scriptDir/tools
libSO=$apkExplodeDir/lib/armeabi-v7a/libAuterionGS.so
symbolFile=$symbolsDir/libAuterionGS.so.sym

if [ -d $apkExplodeDir ]; then
    rm -rf $apkExplodeDir
fi

unzip $apkFile -d $apkExplodeDir
mkdir $symbolsDir

$toolsDir/dump_syms $libSO > $symbolFile

sym_info=`head -n1 $symbolFile`

sym_uuid=`echo $sym_info | cut -d ' ' -f 4`
pdb_file=`echo $sym_info | cut -d ' ' -f 5`

sym_dir=$symbolsDir/$pdb_file/$sym_uuid

echo $sym_dir
mkdir -p $sym_dir

mv $symbolFile $sym_dir

$toolsDir/minidump_stackwalk $dumpFile $symbolsDir > $stackFile 2>$stackWalkLog
