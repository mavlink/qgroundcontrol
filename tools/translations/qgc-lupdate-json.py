#!/usr/bin/env python
import codecs
import json
import os
import sys

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


def escapeXmlString(xmlStr):
    # Escape the string so it can be used in XML
    xmlStr = xmlStr.replace("&", "&amp;")
    xmlStr = xmlStr.replace("<", "&lt;")
    xmlStr = xmlStr.replace(">", "&gt;")
    xmlStr = xmlStr.replace("'", "&apos;")
    xmlStr = xmlStr.replace('"', "&quot;")
    return xmlStr


def writeJsonTSFile(multiFileLocArray):
    with codecs.open("translations/qgc-json.ts", "w", "utf-8") as jsonTSFile:
        jsonTSFile.write('<?xml version="1.0" encoding="utf-8"?>\n')
        jsonTSFile.write("<!DOCTYPE TS>\n")
        jsonTSFile.write('<TS version="2.1">\n')
        for entry in multiFileLocArray:
            jsonTSFile.write("<context>\n")
            jsonTSFile.write(f"    <name>{entry[0]}</name>\n")
            singleFileLocStringDict = entry[2]
            for locStr in singleFileLocStringDict:
                disambiguation = ""
                if locStr.startswith(disambiguationPrefix):
                    workStr = locStr[len(disambiguationPrefix) :]
                    terminatorIndex = workStr.find("#")
                    if terminatorIndex == -1:
                        print(f"Bad disambiguation {entry[0]} '{locStr}'")
                        sys.exit(1)
                    disambiguation = workStr[:terminatorIndex]
                    locStr = workStr[terminatorIndex + 1 :]
                jsonTSFile.write("    <message>\n")
                if len(disambiguation):
                    jsonTSFile.write(f"        <comment>{disambiguation}</comment>\n")
                extraCommentStr = ""
                for jsonHierachy in singleFileLocStringDict[locStr]:
                    extraCommentStr += f"{jsonHierachy}, "
                locStr = escapeXmlString(locStr)
                jsonTSFile.write(f"        <extracomment>{extraCommentStr}</extracomment>\n")
                if extraCommentStr.endswith(".enumStrings, "):
                    jsonTSFile.write(
                        "        <translatorcomment>Only use english comma &apos;,&apos; to separate strings</translatorcomment>\n"
                    )
                jsonTSFile.write(f'        <location filename="{entry[1]}"/>\n')
                jsonTSFile.write(f"        <source>{locStr}</source>\n")
                jsonTSFile.write('        <translation type="unfinished"></translation>\n')
                jsonTSFile.write("    </message>\n")
            jsonTSFile.write("</context>\n")
        jsonTSFile.write("</TS>\n")


def main():
    multiFileLocArray = []
    walkDirectoryTreeForJsonFiles("../src", multiFileLocArray)
    writeJsonTSFile(multiFileLocArray)


if __name__ == "__main__":
    main()
