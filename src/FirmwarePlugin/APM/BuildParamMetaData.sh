cd ~/repos/ardupilot
rm apm.pdef.xml
./Tools/autotest/param_metadata/param_parse.py --vehicle ArduCopter
cp apm.pdef.xml ~/repos/qgroundcontrol/src/FirmwarePlugin/APM/APMParameterFactMetaData.Copter.3.7.xml
rm apm.pdef.xml
cd ~/repos/qgroundcontrol/src/FirmwarePlugin/APM
