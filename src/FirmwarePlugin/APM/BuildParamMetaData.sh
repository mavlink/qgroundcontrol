# Args: [ArduCopter|ArduPlane] [Copter.3.7|...]
cd ~/repos/ardupilot
rm -f apm.pdef.xml
./Tools/autotest/param_metadata/param_parse.py --vehicle $1
cp apm.pdef.xml ~/repos/qgroundcontrol/src/FirmwarePlugin/APM/APMParameterFactMetaData.$2.xml
rm apm.pdef.xml
cd ~/repos/qgroundcontrol/src/FirmwarePlugin/APM
