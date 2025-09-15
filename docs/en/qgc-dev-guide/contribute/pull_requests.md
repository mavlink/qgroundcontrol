# Pull Requests

All pull requests go through the QGC CI build system which builds release and debug version. Builds will fail if there are compiler warnings. Also unit tests are run against supported OS debug builds.

## Automatic Release Note Generation

Releases notes are generated from the following GitHub labels qhich should be set on Pull Requests as appropriate:

* "RN: MAJOR FEATURE"
* "RN: MINOR FEATURE"
* "RN: IMPROVEMENT"
* "RN: REFACTORING"
* "RN: BUGFIX"
* "RN: NEW BOARD SUPPORT"

There are also a set of the above labels which end in "- CUSTOM BUILD" which indicate the changes is associated with the custom build architecture.