#!/usr/bin/env python
import json
import os
import sys
import xml.etree.ElementTree as ET

qgcFileTypeKey = "fileType"
translateKeysKey = "translateKeys"
arrayIDKeysKey = "arrayIDKeys"
disambiguationPrefix = "#loc.disambiguation#"


def parseJsonObjectForTranslateKeys(
    jsonObjectHierarchy, jsonObject, translateKeys, arrayIDKeys, locStringDict
):
    for translateKey in translateKeys:
        if translateKey in jsonObject:
            locStr = jsonObject[translateKey]
            currentHierarchy = jsonObjectHierarchy + "." + translateKey
            if locStr in locStringDict:
                # Duplicate of an existing string
                locStringDict[locStr].append(currentHierarchy)
            else:
                # First time we are seeing this string
                locStringDict[locStr] = [currentHierarchy]
    for key in jsonObject:
        currentHierarchy = jsonObjectHierarchy + "." + key
        if isinstance(jsonObject[key], dict):
            parseJsonObjectForTranslateKeys(
                currentHierarchy,
                jsonObject[key],
                translateKeys,
                arrayIDKeys,
                locStringDict,
            )
        elif isinstance(jsonObject[key], list):
            parseJsonArrayForTranslateKeys(
                currentHierarchy,
                jsonObject[key],
                translateKeys,
                arrayIDKeys,
                locStringDict,
            )


def parseJsonArrayForTranslateKeys(
    jsonObjectHierarchy, jsonArray, translateKeys, arrayIDKeys, locStringDict
):
    for index in range(0, len(jsonArray)):
        jsonObject = jsonArray[index]
        arrayIndexStr = str(index)
        for arrayIDKey in arrayIDKeys:
            if arrayIDKey in jsonObject:
                arrayIndexStr = jsonObject[arrayIDKey]
                break
        currentHierarchy = jsonObjectHierarchy + "[" + arrayIndexStr + "]"
        parseJsonObjectForTranslateKeys(
            currentHierarchy, jsonObject, translateKeys, arrayIDKeys, locStringDict
        )


def addLocKeysBasedOnQGCFileType(jsonPath, jsonDict):
    # Instead of having to add the same keys over and over again in a pile of files we add them here automatically based on file type
    if qgcFileTypeKey in jsonDict:
        qgcFileType = jsonDict[qgcFileTypeKey]
        translateKeyValue = ""
        arrayIDKeysKeyValue = ""
        if qgcFileType == "MavCmdInfo":
            translateKeyValue = "label,enumStrings,friendlyName,description,category"
            arrayIDKeysKeyValue = "rawName,comment"
        elif qgcFileType == "FactMetaData":
            translateKeyValue = "shortDesc,longDesc,enumStrings"
            arrayIDKeysKeyValue = "name"
        if translateKeysKey not in jsonDict and translateKeyValue != "":
            jsonDict[translateKeysKey] = translateKeyValue
        if arrayIDKeysKey not in jsonDict and arrayIDKeysKeyValue != "":
            jsonDict[arrayIDKeysKey] = arrayIDKeysKeyValue


def parseJson(jsonPath, locStringDict):
    with open(jsonPath, "rb") as jsonFile:
        jsonDict = json.load(jsonFile)
    if not isinstance(jsonDict, dict):
        return
    addLocKeysBasedOnQGCFileType(jsonPath, jsonDict)
    if translateKeysKey not in jsonDict:
        return
    translateKeys = jsonDict[translateKeysKey].split(",")
    arrayIDKeys = jsonDict.get(arrayIDKeysKey, "").split(",")
    parseJsonObjectForTranslateKeys("", jsonDict, translateKeys, arrayIDKeys, locStringDict)


def walkDirectoryTreeForJsonFiles(dir, multiFileLocArray):
    for filename in os.listdir(dir):
        path = os.path.join(dir, filename)
        if os.path.isfile(path) and filename.endswith(".json"):
            # print "json",path
            singleFileLocStringDict = {}
            parseJson(path, singleFileLocStringDict)
            if len(singleFileLocStringDict.keys()):
                # Check for duplicate file names
                for entry in multiFileLocArray:
                    if entry[0] == filename:
                        print(f"Error: Duplicate filenames: {filename} paths: {path} {entry[1]}")
                        sys.exit(1)
                multiFileLocArray.append([filename, path, singleFileLocStringDict])
        if os.path.isdir(path):
            walkDirectoryTreeForJsonFiles(path, multiFileLocArray)


def writeJsonTSFile(multiFileLocArray):
    ts_root = ET.Element("TS", version="2.1")

    for entry in multiFileLocArray:
        context = ET.SubElement(ts_root, "context")
        ET.SubElement(context, "name").text = entry[0]

        singleFileLocStringDict = entry[2]
        for locKey in singleFileLocStringDict:
            sourceStr = locKey
            disambiguation = ""
            if sourceStr.startswith(disambiguationPrefix):
                workStr = sourceStr[len(disambiguationPrefix) :]
                terminatorIndex = workStr.find("#")
                if terminatorIndex == -1:
                    print(f"Bad disambiguation {entry[0]} '{sourceStr}'")
                    sys.exit(1)
                disambiguation = workStr[:terminatorIndex]
                sourceStr = workStr[terminatorIndex + 1 :]

            message = ET.SubElement(context, "message")
            if len(disambiguation):
                ET.SubElement(message, "comment").text = disambiguation

            extraCommentStr = ""
            for jsonHierachy in singleFileLocStringDict[locKey]:
                extraCommentStr += f"{jsonHierachy}, "
            ET.SubElement(message, "extracomment").text = extraCommentStr

            if extraCommentStr.endswith(".enumStrings, "):
                ET.SubElement(
                    message, "translatorcomment"
                ).text = "Only use english comma ',' to separate strings"

            ET.SubElement(message, "location", filename=entry[1])
            ET.SubElement(message, "source").text = sourceStr
            ET.SubElement(message, "translation", type="unfinished")

    with open("translations/qgc-json.ts", "w", encoding="utf-8") as jsonTSFile:
        jsonTSFile.write('<?xml version="1.0" encoding="utf-8"?>\n')
        jsonTSFile.write("<!DOCTYPE TS>\n")
        jsonTSFile.write(ET.tostring(ts_root, encoding="unicode"))
        jsonTSFile.write("\n")


def main():
    multiFileLocArray = []
    walkDirectoryTreeForJsonFiles("../src", multiFileLocArray)
    writeJsonTSFile(multiFileLocArray)


if __name__ == "__main__":
    main()
