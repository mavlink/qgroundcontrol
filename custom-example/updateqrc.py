#!/usr/bin/env python
import os

qgc_rc  = "qgroundcontrol.qrc"
res_rc  = "qgcresources.qrc"
qgc_exc = "qgroundcontrol.exclusion"
res_exc = "qgcresources.exclusion"

def read_file(filename):
    with open(filename) as src:
        return [line.rstrip().lstrip() for line in src.readlines()]

def process(src, exclusion, dst):
    file1 = read_file(src)
    file2 = read_file(exclusion)
    file3 = open(dst, 'w')
    for line in file1:
        if line not in file2:
            if line.startswith('<qresource') or line.startswith('</qresource'):
                file3.write('\t')
            if line.startswith('<file') or line.startswith('</file'):
                file3.write('\t\t')
            newLine = str(line)
            if line.startswith('<file'):
                newLine = newLine.replace(">", ">../", 1)
            file3.write(newLine + '\n')
        else:
            print 'Excluded:', line
    file3.close()

def main():
    if (os.path.isfile(qgc_exc)):
        qrc_path = os.path.join("../", qgc_rc)
        if (!os.path.exists(qrc_path)):
            qrc_path = os.path.join("./qgroundcontrol/", qgc_rc)
        process(qrc_path, qgc_exc, qgc_rc)
    if (os.path.isfile(res_exc)):
        res_path = os.path.join("../", res_rc)
        if (!os.path.exists(res_path)):
            res_path = os.path.join("./qgroundcontrol/", res_rc)
        process(res_path, res_exc, res_rc)

if __name__ == '__main__':
    main()
