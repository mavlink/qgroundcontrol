#!/usr/bin/env python
import os

qgc_rc  = "qgroundcontrol.qrc"
res_rc  = "qgcresources.qrc"
qgc_exc = "qgroundcontrol.exclusion"
res_exc = "qgcresources.exclusion"

def read_file(filename):
    with open(filename) as src:
        return [line.rstrip() for line in src.readlines()]

def process(src, exclusion, dst):
    file1 = read_file(src)
    file2 = read_file(exclusion)
    file3 = open(dst, 'w')
    for line in file1:
        if line not in file2:
            newLine = str(line)
            if line.strip().startswith('<file'):
                newLine = newLine.replace(">", ">../", 1)
            file3.write(newLine + '\n')
        else:
            print 'Excluded:', line
    file3.close()

def main():
    if(os.path.isfile(qgc_exc)):
        process(os.path.join("../",qgc_rc), qgc_exc, qgc_rc)
    if(os.path.isfile(res_exc)):
        process(os.path.join("../",res_rc), res_exc, res_rc)

if __name__ == '__main__':
    main()
