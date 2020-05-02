# QGroundControl string translations

QGC uses the standard Qt Linguist mechanisms for string translation. QGC uses crowd sourced string translation through a [Crowdin project](https://crowdin.com/project/qgroundcontrol for translation).


## C++ and Qml code strings
These are coded using the standard Qt tr() for C++ and qsTr() for Qml mechanisms.

## Translating strings within Json files
QGC uses json files internally for metadata. These files need to be translated as well. There is a [python parser](https://github.com/mavlink/qgroundcontrol/blob/master/localization/qgc-lupdate-json.py) which is used to find all the json files in the source tree and pull all the strings out for translation. This parser outputs the localization file for json strings in Qt .ts file format.

In order for the parser to know which strings must be translated additional keys must be available at the root object level.

> Important: Json files which have the same name are not allowed. Since the name is used as the context for the translation lookup it must be unique. The parse will throw an error if it finds duplicate file names.

> Important: The root file name for the json file must be the same as the root filename for the Qt resource alias. This due to the fact that the root filename is used as the translation context. The json parser reads files from the file system and sees file system names. Whereas the QGC C++ code reads json files from the QT resource system and see the file alias as the full path and root name.

### Specifying known file type
The parser supports two known file types: "MAVCmdInfo" and "FactMetaData". If your json file is one of these types you should place a `fileType` key at the root with one of these values. This will cause the parser to use these defaults for instructions:

#### MAVCmdInfo
```
    "translateKeys":    "label,enumStrings,friendlyName,description",
    "arrayIDKeys":      "rawName,comment"
```
#### FactMetaData
```
    "translateKeys":    "shortDescription,longDescription,enumStrings"
    "arrayIDKeys":      "name"
```

### Manually specify parser instructions
For this case dont include the `fileType` key/value pair. And include the followings keys (as needed) in the root object:

* `translateKeys` This key is a string which is a list of all the keys which should be translated.  
* `arrayIDKeys` The json localization parser provides additional information to the translator about where this string came from in the json hierarchy. If there is an array in the json, just displaying an array index as to where this came from is not that helpful. In most cases there is a key within each array element for which the value is unique. If this is the case then specify this key name(s) as the value for `arrayIDKeys`.

### Disambiguation
This is used when you have two strings in the same file which are equal, but there meaning ar different enough that when translated they may each have their own different translation. In order to specific that you include a special prefix marker in the string which includes comments to the translator to explain the specifics of the string.

```
    "foo": "#loc.disambiguation#This is the foo version of baz#baz"
    "bar": "#loc.disambiguation#This is the bar version of baz#baz"
```

In the example above "baz" is the string which is the same for two different keys. The prefix `#loc.disambiguation#` indicates a disambiguation is to follow which is the string between the next set of `#`s.

## Uploading new strings to Crowdin 
To generate (or update) a source translation file, use the [to-crowdin.sh](https://github.com/mavlink/qgroundcontrol/blob/master/localization/to-crowdin.sh) script in this directory. You will need to update the path to Qt within it. This will do the following steps:

* Delete the current qgc.ts file
* Run the qt lupdate command to generate a new qgc.ts file
* Run the python json parser which will add the json strings to the qgc.ts file

Once this is complete you can upload the new qgc.ts file to Crowdin.

https://github.com/mavlink/qgroundcontrol/blob/master/localization/to-crowdin.sh

## Download translations from Crowdin

Once translations have been done/updated, within Crowdin "Build and Download". Extract the resulting qgroundcontro.zip here and run [from-crowdin.sh](https://github.com/mavlink/qgroundcontrol/blob/master/localization/from-crowdin.sh).

This will parse all the source files and generate a language translation file called qgc.ts, which should be uploaded to crowdin.


