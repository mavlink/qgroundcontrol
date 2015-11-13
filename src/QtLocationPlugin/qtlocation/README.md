# QGroundControl

## Qt Location Headers for Qt 5.5.1

Qt version 5.5.1 did not include the headers for QtLocation. It's included here so we can build our location plugin. This is only needed for Qt 5.5.1. The qmake file handles it automatically. Once we move past 5.5.1 we can delete this directory.

The bug has been reported here:

https://bugreports.qt.io/browse/QTBUG-48812
