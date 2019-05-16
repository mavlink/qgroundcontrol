# Working with crash dump files

## Filing crash Issues

* Two files must be provided with the issue (Assuming Android here):
    * The crash .dmp file(s) which can be found in the ```CrashLogs``` directory
    * The .apk file installed on the device. This must match the build which is crashing.

## Creating the stack trace (devs only)

* Must be done using Linux
* Create a directory to hold the crash files
* Place a single crash .dmp file and the .apk into the created directory
* Rename the .dmp file to crash.dmp
* Rename the .apk to crash.apk
* Run ```create_android_sym_file.sh``` with the crash directory you created as a command line option
* The stack trace can be found in ```stackWalk.txt```
* The log for the run can be found in ```stackWalk.log```