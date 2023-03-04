#!/usr/bin/env python
import os

def main():
    qrcFile = open("InstrumentValueIcons.qrc", 'wt')

    qrcFile.write("<RCC>\n")
    qrcFile.write("\t<qresource prefix=\"/InstrumentValueIcons\">\n")

    files = os.listdir("../resources/InstrumentValueIcons")
    for filename in files:
        if filename.endswith(".svg"):
            qrcFile.write("\t\t<file alias=\"%s\">../resources/InstrumentValueIcons/%s</file>\n" % (filename, filename))

    qrcFile.write("\t</qresource>\n")
    qrcFile.write("</RCC>\n")

    qrcFile.close()

if __name__ == '__main__':
    main()
