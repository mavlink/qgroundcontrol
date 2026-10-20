#include "PX4SerialPortConfigTest.h"

#include "PX4SerialPortConfig.h"

UT_REGISTER_TEST_LIGHTWEIGHT(PX4SerialPortConfigTest, TestLabel::Unit)

namespace {

using namespace PX4SerialPortConfig;

Function makeTestFunction(const QString& paramName, const QList<int>& portIds, int currentPortId)
{
    QVariantList enumValues{0};
    QStringList enumStrings{QStringLiteral("Disabled")};
    for (const int portId : portIds) {
        enumValues.append(portId);
        enumStrings.append(QStringLiteral("Port %1").arg(portId));
    }
    return makeFunction(paramName, QString(), enumValues, enumStrings, currentPortId);
}

QList<Function> testFunctions()
{
    return {
        makeTestFunction(QStringLiteral("MAV_0_CONFIG"), {101, 102, 201, 1000}, 101),
        makeTestFunction(QStringLiteral("MAV_1_CONFIG"), {101, 102, 201, 300, 1000}, 0),
        makeTestFunction(QStringLiteral("GPS_1_CONFIG"), {101, 102, 201}, 201),
        makeTestFunction(QStringLiteral("GPS_2_CONFIG"), {101, 102, 201}, 201),
        makeTestFunction(QStringLiteral("UXRCE_DDS_CFG"), {101, 102, 1000}, 1000),
        // Disabled, so it runs on the RC port
        makeTestFunction(QStringLiteral("RC_PORT_CONFIG"), {102, 300}, 0),
    };
}

QStringList writesToStrings(const QList<ParamWrite>& writes)
{
    QStringList strings;
    for (const ParamWrite& write : writes) {
        strings.append(QStringLiteral("%1=%2").arg(write.paramName).arg(write.value));
    }
    return strings;
}

}  // namespace

void PX4SerialPortConfigTest::_functionParamDetection_data()
{
    QTest::addColumn<QVariantList>("enumValues");
    QTest::addColumn<bool>("expected");

    QTest::newRow("serialAndEthernet") << QVariantList{0, 6, 101, 102, 201, 300, 1000} << true;
    QTest::newRow("serialOnly") << QVariantList{0, 101, 201} << true;
    // Small enums such as modes overlap UART indices 1-9
    QTest::newRow("smallEnum") << QVariantList{0, 1, 2, 3} << false;
    QTest::newRow("noDisabled") << QVariantList{101, 102} << false;
    QTest::newRow("nonPortValue") << QVariantList{0, 101, 555} << false;
    QTest::newRow("noEnum") << QVariantList{} << false;
}

void PX4SerialPortConfigTest::_functionParamDetection()
{
    QFETCH(QVariantList, enumValues);
    QFETCH(bool, expected);

    QCOMPARE(isFunctionParam(enumValues), expected);
}

void PX4SerialPortConfigTest::_baudParamName_data()
{
    QTest::addColumn<int>("portId");
    QTest::addColumn<QString>("expected");

    QTest::newRow("uart6") << 6 << QStringLiteral("SER_URT6_BAUD");
    QTest::newRow("telem1") << 101 << QStringLiteral("SER_TEL1_BAUD");
    QTest::newRow("telem4") << 104 << QStringLiteral("SER_TEL4_BAUD");
    QTest::newRow("gps3") << 203 << QStringLiteral("SER_GPS3_BAUD");
    QTest::newRow("rc") << 300 << QStringLiteral("SER_RC_BAUD");
    QTest::newRow("wifi") << 301 << QStringLiteral("SER_WIFI_BAUD");
    QTest::newRow("ext2") << 401 << QStringLiteral("SER_EXT2_BAUD");
    QTest::newRow("ethernet") << kEthernetPortId << QString();
    QTest::newRow("unknown") << 999 << QString();
}

void PX4SerialPortConfigTest::_baudParamName()
{
    QFETCH(int, portId);
    QFETCH(QString, expected);

    QCOMPARE(baudParamName(portId), expected);
}

void PX4SerialPortConfigTest::_functionLabel_data()
{
    QTest::addColumn<QString>("shortDescription");
    QTest::addColumn<QString>("expected");

    QTest::newRow("px4Description") << QStringLiteral("Serial Configuration for MAVLink (instance 1)")
                                    << QStringLiteral("MAVLink (instance 1)");
    QTest::newRow("otherDescription") << QStringLiteral("Custom port") << QStringLiteral("Custom port");
    QTest::newRow("noDescription") << QString() << QStringLiteral("MAV_1_CONFIG");
}

void PX4SerialPortConfigTest::_functionLabel()
{
    QFETCH(QString, shortDescription);
    QFETCH(QString, expected);

    QCOMPARE(functionLabel(QStringLiteral("MAV_1_CONFIG"), shortDescription), expected);
}

void PX4SerialPortConfigTest::_portsAreUnionOfFunctionPortsInIdOrder()
{
    const QList<Function> functions = {
        makeFunction(QStringLiteral("MAV_0_CONFIG"), QString(), {0, 102, 1000},
                     {QStringLiteral("Disabled"), QStringLiteral("TELEM 2"), QStringLiteral("Ethernet")}, 0),
        makeFunction(QStringLiteral("GPS_1_CONFIG"), QString(), {0, 201, 101},
                     {QStringLiteral("Disabled"), QStringLiteral("GPS 1"), QStringLiteral("TELEM 1")}, 0),
    };

    const QList<Port> result = ports(functions);

    QStringList labels;
    QList<int> ids;
    for (const Port& port : result) {
        ids.append(port.id);
        labels.append(port.label);
    }
    QCOMPARE(ids, (QList<int>{101, 102, 201, kEthernetPortId}));
    QCOMPARE(labels, (QStringList{QStringLiteral("TELEM 1"), QStringLiteral("TELEM 2"), QStringLiteral("GPS 1"),
                                  QStringLiteral("Ethernet")}));
}

void PX4SerialPortConfigTest::_functionsOnPort()
{
    const QList<Function> functions = testFunctions();

    QCOMPARE(functionsOnPort(functions, 101), QStringList{QStringLiteral("MAV_0_CONFIG")});
    QCOMPARE(functionsOnPort(functions, 201),
             (QStringList{QStringLiteral("GPS_1_CONFIG"), QStringLiteral("GPS_2_CONFIG")}));
    QCOMPARE(functionsOnPort(functions, 102), QStringList());
}

void PX4SerialPortConfigTest::_rcInputDriverDefaultsToRcPort()
{
    // PX4 always starts RC Input Driver. Disabled makes it use the board's default port, normally the RC port.
    const Function withRcPort = makeTestFunction(QStringLiteral("RC_PORT_CONFIG"), {102, 300}, kDisabledPortId);
    QCOMPARE(withRcPort.currentPortId, 300);
    QCOMPARE(withRcPort.defaultPortId, 300);

    const Function withoutRcPort = makeTestFunction(QStringLiteral("RC_PORT_CONFIG"), {102}, kDisabledPortId);
    QCOMPARE(withoutRcPort.currentPortId, kDisabledPortId);

    const Function otherFunction = makeTestFunction(QStringLiteral("MAV_1_CONFIG"), {102, 300}, kDisabledPortId);
    QCOMPARE(otherFunction.currentPortId, kDisabledPortId);
}

void PX4SerialPortConfigTest::_functionsForPortListUsualFunctionsFirst_data()
{
    QTest::addColumn<int>("portId");
    QTest::addColumn<QStringList>("expected");

    // Functions are given in parameter name order: GPS_1, MAV_0, RC_CRSF, RC_PORT, SEP_PORT1, TEL_FRSKY, UXRCE_DDS
    QTest::newRow("gps") << 201 << QStringList{QStringLiteral("GPS_1_CONFIG"),   QStringLiteral("SEP_PORT1_CFG"),
                                               QStringLiteral("MAV_0_CONFIG"),   QStringLiteral("RC_CRSF_PRT_CFG"),
                                               QStringLiteral("RC_PORT_CONFIG"), QStringLiteral("TEL_FRSKY_CONFIG"),
                                               QStringLiteral("UXRCE_DDS_CFG")};
    QTest::newRow("telem") << 102 << QStringList{QStringLiteral("MAV_0_CONFIG"),    QStringLiteral("UXRCE_DDS_CFG"),
                                                 QStringLiteral("GPS_1_CONFIG"),    QStringLiteral("RC_CRSF_PRT_CFG"),
                                                 QStringLiteral("RC_PORT_CONFIG"),  QStringLiteral("SEP_PORT1_CFG"),
                                                 QStringLiteral("TEL_FRSKY_CONFIG")};
    QTest::newRow("rc") << 300 << QStringList{QStringLiteral("RC_CRSF_PRT_CFG"), QStringLiteral("RC_PORT_CONFIG"),
                                              QStringLiteral("GPS_1_CONFIG"),    QStringLiteral("MAV_0_CONFIG"),
                                              QStringLiteral("SEP_PORT1_CFG"),   QStringLiteral("TEL_FRSKY_CONFIG"),
                                              QStringLiteral("UXRCE_DDS_CFG")};
    QTest::newRow("noUsualFunctions") << 6
                                      << QStringList{
                                             QStringLiteral("GPS_1_CONFIG"),    QStringLiteral("MAV_0_CONFIG"),
                                             QStringLiteral("RC_CRSF_PRT_CFG"), QStringLiteral("RC_PORT_CONFIG"),
                                             QStringLiteral("SEP_PORT1_CFG"),   QStringLiteral("TEL_FRSKY_CONFIG"),
                                             QStringLiteral("UXRCE_DDS_CFG")};
    QTest::newRow("onlyAllowedFunctions")
        << kEthernetPortId << QStringList{QStringLiteral("MAV_0_CONFIG"), QStringLiteral("UXRCE_DDS_CFG")};
}

void PX4SerialPortConfigTest::_functionsForPortListUsualFunctionsFirst()
{
    QFETCH(int, portId);
    QFETCH(QStringList, expected);

    const QList<int> serialPorts{6, 102, 201, 300};
    const QList<int> networkPorts{6, 102, 201, 300, kEthernetPortId};
    const QList<Function> functions = {
        makeTestFunction(QStringLiteral("GPS_1_CONFIG"), serialPorts, 0),
        makeTestFunction(QStringLiteral("MAV_0_CONFIG"), networkPorts, 0),
        makeTestFunction(QStringLiteral("RC_CRSF_PRT_CFG"), serialPorts, 0),
        makeTestFunction(QStringLiteral("RC_PORT_CONFIG"), serialPorts, 0),
        makeTestFunction(QStringLiteral("SEP_PORT1_CFG"), serialPorts, 0),
        makeTestFunction(QStringLiteral("TEL_FRSKY_CONFIG"), serialPorts, 0),
        makeTestFunction(QStringLiteral("UXRCE_DDS_CFG"), networkPorts, 0),
    };

    QStringList names;
    for (const Function& function : functionsForPort(functions, portId)) {
        names.append(function.paramName);
    }
    QCOMPARE(names, expected);
}

void PX4SerialPortConfigTest::_assignPort_data()
{
    QTest::addColumn<int>("portId");
    QTest::addColumn<QString>("functionParamName");
    QTest::addColumn<QStringList>("expectedWrites");

    QTest::newRow("assignToEmptyPort") << 102 << QStringLiteral("MAV_1_CONFIG")
                                       << QStringList{QStringLiteral("MAV_1_CONFIG=102")};
    QTest::newRow("moveFromOtherPort") << 102 << QStringLiteral("MAV_0_CONFIG")
                                       << QStringList{QStringLiteral("MAV_0_CONFIG=102")};
    QTest::newRow("replaceExisting") << 101 << QStringLiteral("MAV_1_CONFIG")
                                     << QStringList{QStringLiteral("MAV_0_CONFIG=0"),
                                                    QStringLiteral("MAV_1_CONFIG=101")};
    QTest::newRow("clearPort") << 101 << QString() << QStringList{QStringLiteral("MAV_0_CONFIG=0")};
    QTest::newRow("resolveConflict") << 201 << QStringLiteral("GPS_1_CONFIG")
                                     << QStringList{QStringLiteral("GPS_2_CONFIG=0")};
    QTest::newRow("alreadyAssigned") << 101 << QStringLiteral("MAV_0_CONFIG") << QStringList();
    QTest::newRow("portNotAllowed") << kEthernetPortId << QStringLiteral("GPS_1_CONFIG") << QStringList();
    QTest::newRow("unknownFunction") << 102 << QStringLiteral("XXX_CONFIG") << QStringList();
    // Several functions can share Ethernet, so assigning one there keeps the others
    QTest::newRow("sharedPortKeepsOthers")
        << kEthernetPortId << QStringLiteral("MAV_1_CONFIG") << QStringList{QStringLiteral("MAV_1_CONFIG=1000")};
    // Disabling RC Input Driver doesn't free its default port, so it is left alone
    QTest::newRow("rcDefaultPortKeepsRcInput")
        << 300 << QStringLiteral("MAV_1_CONFIG") << QStringList{QStringLiteral("MAV_1_CONFIG=300")};
    QTest::newRow("rcDefaultPortNone") << 300 << QString() << QStringList();
    QTest::newRow("rcMovedOffDefaultPort")
        << 102 << QStringLiteral("RC_PORT_CONFIG") << QStringList{QStringLiteral("RC_PORT_CONFIG=102")};
}

void PX4SerialPortConfigTest::_assignPort()
{
    QFETCH(int, portId);
    QFETCH(QString, functionParamName);
    QFETCH(QStringList, expectedWrites);

    QCOMPARE(writesToStrings(assignPort(testFunctions(), portId, functionParamName)), expectedWrites);
}

void PX4SerialPortConfigTest::_removeFromPort_data()
{
    QTest::addColumn<int>("portId");
    QTest::addColumn<QString>("functionParamName");
    QTest::addColumn<QStringList>("expectedWrites");

    QTest::newRow("onPort") << kEthernetPortId << QStringLiteral("UXRCE_DDS_CFG")
                            << QStringList{QStringLiteral("UXRCE_DDS_CFG=0")};
    QTest::newRow("notOnPort") << kEthernetPortId << QStringLiteral("MAV_1_CONFIG") << QStringList();
    QTest::newRow("onOtherPort") << 102 << QStringLiteral("MAV_0_CONFIG") << QStringList();
    QTest::newRow("unknownFunction") << kEthernetPortId << QStringLiteral("XXX_CONFIG") << QStringList();
    QTest::newRow("rcDefaultPort") << 300 << QStringLiteral("RC_PORT_CONFIG") << QStringList();
}

void PX4SerialPortConfigTest::_removeFromPort()
{
    QFETCH(int, portId);
    QFETCH(QString, functionParamName);
    QFETCH(QStringList, expectedWrites);

    QCOMPARE(writesToStrings(removeFromPort(testFunctions(), portId, functionParamName)), expectedWrites);
}

void PX4SerialPortConfigTest::_optionParamNames_data()
{
    QTest::addColumn<QString>("functionParamName");
    QTest::addColumn<int>("portId");
    QTest::addColumn<QStringList>("expected");

    QTest::newRow("mavlinkSerial") << QStringLiteral("MAV_1_CONFIG") << 102
                                   << QStringList{QStringLiteral("MAV_1_MODE"), QStringLiteral("MAV_1_RATE"),
                                                  QStringLiteral("MAV_1_FORWARD"), QStringLiteral("MAV_1_FLOW_CTRL")};
    QTest::newRow("mavlinkEthernet") << QStringLiteral("MAV_0_CONFIG") << kEthernetPortId
                                     << QStringList{
                                            QStringLiteral("MAV_0_MODE"),       QStringLiteral("MAV_0_RATE"),
                                            QStringLiteral("MAV_0_FORWARD"),    QStringLiteral("MAV_0_UDP_PRT"),
                                            QStringLiteral("MAV_0_REMOTE_PRT"), QStringLiteral("MAV_0_BROADCAST")};
    QTest::newRow("gps") << QStringLiteral("GPS_2_CONFIG") << 202 << QStringList{QStringLiteral("GPS_2_PROTOCOL")};
    QTest::newRow("noOptions") << QStringLiteral("RC_PORT_CONFIG") << 300 << QStringList();
    QTest::newRow("disabled") << QStringLiteral("MAV_1_CONFIG") << kDisabledPortId << QStringList();
}

void PX4SerialPortConfigTest::_optionParamNames()
{
    QFETCH(QString, functionParamName);
    QFETCH(int, portId);
    QFETCH(QStringList, expected);

    QCOMPARE(optionParamNames(functionParamName, portId), expected);
}
