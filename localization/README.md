To generate (or update) a source translation file, use the "lupdate" command line tool. It can be found in the Qt distribution.

(Easiest to just run the `gen_translation_source.sh` script, or get the commands from within the script and run them manually)

This will parse all the source files and generate a language translation file called qgc.ts. This file should be copied into whatever language it is to be translated to and sent to translation. The translation can either be done directly within the (XML) file, using Qt's "Linguist" tool, or uploaded to crowdin.

For instance, to localize to Germany German as an example you first copy the original source:
```
cp qgc.ts qgc_de-DE.ts
```
The German localization is then performed in qgc_de-DE.ts

Once localization is complete, you "compile" it using the "lrelease" command line tool.
```
lrelease qgc_de-DE.ts
```
This will generate the ("compiled") localization file `qgc_de-DE.qm`, which should be shipped with QGroundControl.

Further documentation can be found at:

http://doc.qt.io/qt-5/qtlinguist-index.html

Note about crowdin:

If you build the project and download the resulting ZIP file, the translated (.ts) files all come with the same name "qgc.ts". They each reside in a different directory named after the language for which it was translated into. Care must be taken to rename these files before moving them to the locale directory so you don't override the original, English source file.
