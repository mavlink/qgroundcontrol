# Unit Tests

_QGroundControl_ (QGC) contains a set of unit tests that must pass before a pull request will be accepted. The addition of new complex subsystems to QGC should include a corresponding new unit test to test it.

The full list of unit tests can be found in [UnitTestList.cc](https://github.com/mavlink/qgroundcontrol/blob/master/src/qgcunittest/UnitTestList.cc).

To run unit tests:

1. Build in `debug` mode with `UNITTEST_BUILD` definition.
1. Copy the **deploy/qgroundcontrol-start.sh** script in the **debug** directory
1. Run _all_ unit tests from the command line using the `--unittest` command line option.
   For Linux this is done as shown:
   ```
   qgroundcontrol-start.sh --unittest
   ```
1. Run _individual_ unit tests by specifying the test name as well: `--unittest:RadioConfigTest`.
   For Linux this is done as shown:
   ```
   qgroundcontrol-start.sh --unittest:RadioConfigTest
   ```
