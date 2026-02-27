#!/usr/bin/env python
import os


def main():
    with open("InstrumentValueIcons.qrc", "w") as qrcFile:
        qrcFile.write("<RCC>\n")
        qrcFile.write('\t<qresource prefix="/InstrumentValueIcons">\n')

        files = os.listdir(".")
        for filename in files:
            if filename.endswith(".svg"):
                qrcFile.write(f'\t\t<file alias="{filename}">{filename}</file>\n')

        qrcFile.write("\t</qresource>\n")
        qrcFile.write("</RCC>\n")


if __name__ == "__main__":
    main()
