#!/usr/bin/env python
import os
import json
from xml.dom.minidom import parse
import xml.dom.minidom
import codecs
import sys

qgcFileTypeKey =        "fileType"
translateKeysKey =      "translateKeys"
arrayIDKeysKey =        "arrayIDKeys"
disambiguationPrefix =  "#loc.disambiguation#"

def parseJsonObjectForTranslateKeys(jsonObjectHierarchy, jsonObject, translateKeys, arrayIDKeys, locStringDict):
    for translateKey in translateKeys:
        if (translateKey in jsonObject):
            locStr = jsonObject[translateKey]
            currentHierarchy = jsonObjectHierarchy + "." + translateKey
            if locStr in locStringDict:
                # Duplicate of an existing string
                locStringDict[locStr].append(currentHierarchy)
            else:
                # First time we are seeing this string
                locStringDict[locStr] = [ currentHierarchy ]
    for key in jsonObject:
        currentHierarchy = jsonObjectHierarchy + "." + key
        if (type(jsonObject[key]) == type({})):
            parseJsonObjectForTranslateKeys(currentHierarchy, jsonObject[key], translateKeys,arrayIDKeys, locStringDict)
        elif (type(jsonObject[key]) == type([])):
            parseJsonArrayForTranslateKeys(currentHierarchy, jsonObject[key], translateKeys, arrayIDKeys, locStringDict)

def parseJsonArrayForTranslateKeys(jsonObjectHierarchy, jsonArray, translateKeys, arrayIDKeys, locStringDict):
    for index in range(0, len(jsonArray)):
        jsonObject = jsonArray[index]
        arrayIndexStr = str(index)
        for arrayIDKey in arrayIDKeys:
            if arrayIDKey in jsonObject.keys():
                arrayIndexStr = jsonObject[arrayIDKey]
                break
        currentHierarchy = jsonObjectHierarchy + "[" + arrayIndexStr + "]"
        parseJsonObjectForTranslateKeys(currentHierarchy, jsonObject, translateKeys, arrayIDKeys, locStringDict)

def addLocKeysBasedOnQGCFileType(jsonPath, jsonDict):
    # Instead of having to add the same keys over and over again in a pile of files we add them here automatically based on file type
    if qgcFileTypeKey in jsonDict:
        qgcFileType = jsonDict[qgcFileTypeKey]
        translateKeyValue =         ""
        arrayIDKeysKeyValue =       ""
        if qgcFileType == "MavCmdInfo":
            translateKeyValue =         "label,enumStrings,friendlyName,description,category"
            arrayIDKeysKeyValue =       "rawName,comment"
        elif qgcFileType == "FactMetaData":
            translateKeyValue =         "shortDescription,longDescription,enumStrings"
            arrayIDKeysKeyValue =       "name"
        if translateKeysKey not in jsonDict and translateKeyValue != "":
            jsonDict[translateKeysKey] = translateKeyValue
        if arrayIDKeysKey not in jsonDict and arrayIDKeysKeyValue != "":
            jsonDict[arrayIDKeysKey] = arrayIDKeysKeyValue

def parseJson(jsonPath, locStringDict):
    jsonFile = open(jsonPath, "rb")
    jsonDict = json.load(jsonFile)
    if (type(jsonDict) != type({})):
        return
    addLocKeysBasedOnQGCFileType(jsonPath, jsonDict)
    if (not translateKeysKey in jsonDict):
        return
    translateKeys = jsonDict[translateKeysKey].split(",")
    arrayIDKeys = jsonDict.get(arrayIDKeysKey, "").split(",")
    parseJsonObjectForTranslateKeys("", jsonDict, translateKeys, arrayIDKeys, locStringDict)

def walkDirectoryTreeForJsonFiles(dir, multiFileLocArray):
    for filename in os.listdir(dir):
        path = os.path.join(dir, filename)
        if (os.path.isfile(path) and filename.endswith(".json")):
            #print "json",path
            singleFileLocStringDict = {}
            parseJson(path, singleFileLocStringDict)
            if len(singleFileLocStringDict.keys()):
                # Check for duplicate file names
                for entry in multiFileLocArray:
                    if entry[0] == filename:
                        print("Error: Duplicate filenames: %s paths: %s %s" % (filename, path, entry[1]))
                        sys.exit(1)
                multiFileLocArray.append([filename, path, singleFileLocStringDict])
        if (os.path.isdir(path)):
            walkDirectoryTreeForJsonFiles(path, multiFileLocArray)

def writeJsonTSFile(multiFileLocArray):
    jsonTSFile = codecs.open('qgc-json.ts', 'w', "utf-8")
    jsonTSFile.write("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n")
    jsonTSFile.write("<!DOCTYPE TS>\n")
    jsonTSFile.write("<TS version=\"2.1\">\n")
    for entry in multiFileLocArray:
        jsonTSFile.write("<context>\n")
        jsonTSFile.write("    <name>%s</name>\n" % entry[0])
        singleFileLocStringDict = entry[2]
        for locStr in singleFileLocStringDict.keys():
            disambiguation = ""
            if locStr.startswith(disambiguationPrefix):
                workStr = locStr[len(disambiguationPrefix):]
                terminatorIndex = workStr.find("#")
                if terminatorIndex == -1:
                    print("Bad disambiguation %1 '%2'" % (entry[0], locStr))
                    sys.exit(1)
                disambiguation = workStr[:terminatorIndex]
                locStr = workStr[terminatorIndex+1:]
            jsonTSFile.write("    <message>\n")
            if len(disambiguation):
                jsonTSFile.write("        <comment>%s</comment>\n" % disambiguation)
            extraCommentStr = ""
            for jsonHierachy in singleFileLocStringDict[locStr]:
                extraCommentStr += "%s, " % jsonHierachy
            jsonTSFile.write("        <extracomment>%s</extracomment>\n" % extraCommentStr)
            jsonTSFile.write("        <location filename=\"%s\"/>\n" % entry[1])
            jsonTSFile.write("        <source>%s</source>\n" % locStr)
            jsonTSFile.write("        <translation type=\"unfinished\"></translation>\n")
            jsonTSFile.write("    </message>\n")
        jsonTSFile.write("</context>\n")
    jsonTSFile.write("</TS>\n")
    jsonTSFile.close()

def main():
    multiFileLocArray = []
    walkDirectoryTreeForJsonFiles("../src", multiFileLocArray)
    writeJsonTSFile(multiFileLocArray)

if __name__ == '__main__':
    main()
