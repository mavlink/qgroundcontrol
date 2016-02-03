import fileinput
import sys
import os
import glob

def get_actual_filename(name):
    dirs = name.split('\\')
    # disk letter
    test_name = [dirs[0].upper()]
    for d in dirs[1:]:
        test_name += ["%s[%s]" % (d[:-1], d[-1])]
    res = glob.glob('\\'.join(test_name))
    if not res:
        #File not found
        return None
    return res[0]
    
filelist = [get_actual_filename(x.rstrip()) for x in fileinput.input(['-'])]
prefix_len = len(os.path.commonprefix(filelist))

print("""SRCSRV: ini ------------------------------------------------
VERSION=2
SRCSRV: variables ------------------------------------------
SRCSRVVERCTRL=https
SRCSRVTRG=https://raw.github.com/mavlink/qgroundcontrol/%s/%%var2%%
SRCSRV: source files ---------------------------------------""" % os.environ['APPVEYOR_REPO_COMMIT'])
for line in filelist:
    if line is not None:
        print('%s*%s' % (line, line[prefix_len:].replace('\\','/')))
        
print("SRCSRV: end ------------------------------------------------")