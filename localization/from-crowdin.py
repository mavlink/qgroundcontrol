#!/usr/bin/env python
import os
from shutil import copyfile

# When you "Build and Download" from Crowdin, you endup with a qgroundcontro.zip file.
# It is assumed this zip file has been extracted into ./qgroundcontrol

for lang in os.listdir("./qgroundcontrol"):
    srcFile = os.path.join("./qgroundcontrol", lang, "qgc.ts")
    if(os.path.exists(srcFile)):
        lang = lang.replace("-","_")
        dstFile = "./qgc" + "_" + lang + ".ts"
        print("qgc" + "_" + lang + ".ts")
        copyfile(srcFile, dstFile)

