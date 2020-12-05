Windows pre-built libraries come from here: https://tukaani.org/xz/, https://tukaani.org/xz/xz-5.2.5-windows.zip as the starting point:

* To save space only the include files and 64 bit dll/lib are copied to the repo
* The prebuilt version doesn't include a static library so we need to use the dll version.
* The prebuilt version doesn't include a lib to link with the static version so we built it ourselves using this: https://stackoverflow.com/questions/9946322/how-to-generate-an-import-library-lib-file-from-a-dll

dumpbin /exports liblzma.dll> exports.txt
echo LIBRARY LIBLZMA > liblzma.def
echo EXPORTS >> liblzma.def
for /f "skip=19 tokens=4" %A in (exports.txt) do echo %A >> liblzma.def
lib /def:liblzma.def /out:liblzma.lib /machine:x64
