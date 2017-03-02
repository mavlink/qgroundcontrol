/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "MissionItemTest.h"
#include "LinkManager.h"
#include "MultiVehicleManager.h"
#include "MissionItem.h"
#include "SimpleMissionItem.h"

#if 0
const MissionItemTest::TestCase_t MissionItemTest::_rgTestCases[] = {
    { "0\t0\t3\t16\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 0, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_WAYPOINT,     10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "1\t0\t3\t17\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 1, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_UNLIM, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "2\t0\t3\t18\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 2, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TURNS, 10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "3\t0\t3\t19\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 3, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LOITER_TIME,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "4\t0\t3\t21\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n",  { 4, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_NAV_LAND,         10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_GLOBAL_RELATIVE_ALT } },
    { "6\t0\t2\t112\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n", { 5, QGeoCoordinate(-10.0, -20.0, -30.0), MAV_CMD_CONDITION_DELAY,  10.0, 20.0, 30.0, 40.0, true, false, MAV_FRAME_MISSION } },
};
const size_t MissionItemTest::_cTestCases = sizeof(_rgTestCases)/sizeof(_rgTestCases[0]);
#endif

MissionItemTest::MissionItemTest(void)
{
    
}

// Test property get/set
void MissionItemTest::_testSetGet(void)
{
    MissionItem missionItem;

    missionItem.setSequenceNumber(1);
    QCOMPARE(missionItem.sequenceNumber(), 1);

    missionItem.setCommand(MAV_CMD_NAV_WAYPOINT);
    QCOMPARE(missionItem.command(), MAV_CMD_NAV_WAYPOINT);

    missionItem.setFrame(MAV_FRAME_LOCAL_NED);
    QCOMPARE(missionItem.frame(), MAV_FRAME_LOCAL_NED);
    QCOMPARE(missionItem.relativeAltitude(), false);
    missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(missionItem.relativeAltitude(), true);

    missionItem.setParam1(1.0);
    QCOMPARE(missionItem.param1(), 1.0);

    missionItem.setParam2(2.0);
    QCOMPARE(missionItem.param2(), 2.0);

    missionItem.setParam3(3.0);
    QCOMPARE(missionItem.param3(), 3.0);

    missionItem.setParam4(4.0);
    QCOMPARE(missionItem.param4(), 4.0);

    missionItem.setParam5(5.0);
    QCOMPARE(missionItem.param5(), 5.0);

    missionItem.setParam6(6.0);
    QCOMPARE(missionItem.param6(), 6.0);

    missionItem.setParam7(7.0);
    QCOMPARE(missionItem.param7(), 7.0);

    QCOMPARE(missionItem.coordinate(), QGeoCoordinate(5.0, 6.0, 7.0));

    missionItem.setAutoContinue(false);
    QCOMPARE(missionItem.autoContinue(), false);

    missionItem.setIsCurrentItem(true);
    QCOMPARE(missionItem.isCurrentItem(), true);
}

// Test basic signalling
void MissionItemTest::_testSignals(void)
{
    MissionItem missionItem(1,                                  // sequenceNumber
                            MAV_CMD_NAV_WAYPOINT,               // command
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                            1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  // params
                            true,                               // autoContinue
                            true);                              // isCurrentItem

    enum {
        isCurrentItemChangedIndex = 0,
        sequenceNumberChangedIndex,
        maxSignalIndex
    };

    enum {
        isCurrentItemChangedMask =          1 << isCurrentItemChangedIndex,
        sequenceNumberChangedIndexMask =    1 << sequenceNumberChangedIndex
    };

    static const size_t cMissionItemSignals = maxSignalIndex;
    const char*         rgMissionItemSignals[cMissionItemSignals];

    rgMissionItemSignals[isCurrentItemChangedIndex] =   SIGNAL(isCurrentItemChanged(bool));
    rgMissionItemSignals[sequenceNumberChangedIndex] =  SIGNAL(sequenceNumberChanged(int));

    MultiSignalSpy* multiSpyMissionItem = new MultiSignalSpy();
    Q_CHECK_PTR(multiSpyMissionItem);
    QCOMPARE(multiSpyMissionItem->init(&missionItem, rgMissionItemSignals, cMissionItemSignals), true);

    // Validate isCurrentItemChanged signalling
    missionItem.setIsCurrentItem(true);
    QVERIFY(multiSpyMissionItem->checkNoSignals());
    missionItem.setIsCurrentItem(false);
    QVERIFY(multiSpyMissionItem->checkOnlySignalByMask(isCurrentItemChangedMask));
    QSignalSpy* spy = multiSpyMissionItem->getSpyByIndex(isCurrentItemChangedIndex);
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), false);

    multiSpyMissionItem->clearAllSignals();

    // Validate sequenceNumberChanged signalling
    missionItem.setSequenceNumber(1);
    QVERIFY(multiSpyMissionItem->checkNoSignals());
    missionItem.setSequenceNumber(2);
    QVERIFY(multiSpyMissionItem->checkOnlySignalByMask(sequenceNumberChangedIndexMask));
    spy = multiSpyMissionItem->getSpyByIndex(sequenceNumberChangedIndex);
    signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toInt(), 2);
}

// Test signalling associated with contained facts
void MissionItemTest::_testFactSignals(void)
{
    MissionItem missionItem(1,                                  // sequenceNumber
                            MAV_CMD_NAV_WAYPOINT,               // command
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0,  // params
                            true,                               // autoContinue
                            true);                              // isCurrentItem


    // command
    QSignalSpy commandSpy(&missionItem._commandFact, SIGNAL(valueChanged(QVariant)));
    missionItem.setCommand(MAV_CMD_NAV_WAYPOINT);
    QCOMPARE(commandSpy.count(), 0);
    missionItem.setCommand(MAV_CMD_NAV_ALTITUDE_WAIT);
    QCOMPARE(commandSpy.count(), 1);
    QList<QVariant> arguments = commandSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE((MAV_CMD)arguments.at(0).toInt(), MAV_CMD_NAV_ALTITUDE_WAIT);

    // frame
    QSignalSpy frameSpy(&missionItem._frameFact, SIGNAL(valueChanged(QVariant)));
    missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(frameSpy.count(), 0);
    missionItem.setFrame(MAV_FRAME_BODY_NED);
    QCOMPARE(frameSpy.count(), 1);
    arguments = frameSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE((MAV_FRAME)arguments.at(0).toInt(), MAV_FRAME_BODY_NED);

    // param1
    QSignalSpy param1Spy(&missionItem._param1Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam1(1.0);
    QCOMPARE(param1Spy.count(), 0);
    missionItem.setParam1(2.0);
    QCOMPARE(param1Spy.count(), 1);
    arguments = param1Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 2.0);

    // param2
    QSignalSpy param2Spy(&missionItem._param2Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam2(2.0);
    QCOMPARE(param2Spy.count(), 0);
    missionItem.setParam2(3.0);
    QCOMPARE(param2Spy.count(), 1);
    arguments = param2Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 3.0);

    // param3
    QSignalSpy param3Spy(&missionItem._param3Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam3(3.0);
    QCOMPARE(param3Spy.count(), 0);
    missionItem.setParam3(4.0);
    QCOMPARE(param3Spy.count(), 1);
    arguments = param3Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 4.0);

    // param4
    QSignalSpy param4Spy(&missionItem._param4Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam4(4.0);
    QCOMPARE(param4Spy.count(), 0);
    missionItem.setParam4(5.0);
    QCOMPARE(param4Spy.count(), 1);
    arguments = param4Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 5.0);

    // param6
    QSignalSpy param6Spy(&missionItem._param6Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam6(6.0);
    QCOMPARE(param6Spy.count(), 0);
    missionItem.setParam6(7.0);
    QCOMPARE(param6Spy.count(), 1);
    arguments = param6Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 7.0);

    // param7
    QSignalSpy param7Spy(&missionItem._param7Fact, SIGNAL(valueChanged(QVariant)));
    missionItem.setParam7(7.0);
    QCOMPARE(param7Spy.count(), 0);
    missionItem.setParam7(8.0);
    QCOMPARE(param7Spy.count(), 1);
    arguments = param7Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 8.0);
}

void MissionItemTest::_checkExpectedMissionItem(const MissionItem& missionItem)
{
    QCOMPARE(missionItem.sequenceNumber(), _seq);
    QCOMPARE(missionItem.isCurrentItem(), false);
    QCOMPARE(missionItem.frame(), (MAV_FRAME)3);
    QCOMPARE(missionItem.command(), (MAV_CMD)80);
    QCOMPARE(missionItem.param1(), 10.0);
    QCOMPARE(missionItem.param2(), 20.0);
    QCOMPARE(missionItem.param3(), 30.0);
    QCOMPARE(missionItem.param4(), 40.0);
    QCOMPARE(missionItem.param5(), -10.0);
    QCOMPARE(missionItem.param6(), -20.0);
    QCOMPARE(missionItem.param7(), -30.0);
    QCOMPARE(missionItem.autoContinue(), true);
}

void MissionItemTest::_testLoadFromStream(void)
{
    MissionItem missionItem;

    QString testString("10\t0\t3\t80\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n");
    QTextStream testStream(&testString, QIODevice::ReadOnly);

    QVERIFY(missionItem.load(testStream));
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testSimpleLoadFromStream(void)
{
    // We specifically test SimpleMissionItem loading as well since it has additional
    // signalling which can affect values.
    SimpleMissionItem simpleMissionItem(NULL);

    QString testString("10\t0\t3\t80\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n");
    QTextStream testStream(&testString, QIODevice::ReadOnly);

    QVERIFY(simpleMissionItem.load(testStream));
    _checkExpectedMissionItem(simpleMissionItem.missionItem());
}

void MissionItemTest::_testLoadFromJsonV1(void)
{
    MissionItem missionItem;
    QString     errorString;
    QJsonArray  coordinateArray;
    coordinateArray << -10.0 << -20.0 <<-30.0;
    QJsonObject jsonObject;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(MissionItem::_jsonParam1Key, 10);
    jsonObject.insert(MissionItem::_jsonParam2Key, 20);
    jsonObject.insert(MissionItem::_jsonParam3Key, 30);
    jsonObject.insert(MissionItem::_jsonParam4Key, 40);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    jsonObject.insert(MissionItem::_jsonCoordinateKey, coordinateArray);


    // We only need to test the differences between V1 and V2

    QStringList removeKeys;
    removeKeys << MissionItem::_jsonParam1Key << MissionItem::_jsonParam2Key << MissionItem::_jsonParam3Key << MissionItem::_jsonParam4Key;
    foreach (const QString& removeKey, removeKeys) {
        QJsonObject badObject = jsonObject;
        badObject.remove(removeKey);
        QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
        QVERIFY(!errorString.isEmpty());
        qDebug() << errorString;
    }

    // Test good load

    QVERIFY(missionItem.load(jsonObject, _seq, errorString));
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testLoadFromJsonV2(void)
{
    MissionItem missionItem;
    QString     errorString;
    QJsonArray  coordinateArray;
    coordinateArray << -10.0 << -20.0 <<-30.0;
    QJsonObject jsonObject;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    jsonObject.insert(MissionItem::_jsonCoordinateKey, coordinateArray);

    QJsonArray rgParams =  { 10, 20, 30, 40 };
    jsonObject.insert(MissionItem::_jsonParamsKey, rgParams);

    // Test missing key detection

    QStringList removeKeys;
    removeKeys << MissionItem::_jsonAutoContinueKey <<
                  MissionItem::_jsonCommandKey <<
                  MissionItem::_jsonFrameKey <<
                  MissionItem::_jsonParamsKey <<
                  VisualMissionItem::jsonTypeKey <<
                  MissionItem::_jsonCoordinateKey;
    foreach(const QString& removeKey, removeKeys) {
        QJsonObject badObject = jsonObject;
        badObject.remove(removeKey);
        QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
        QVERIFY(!errorString.isEmpty());
        qDebug() << errorString;
    }

    // Test bad coordinate variations

    QJsonObject badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = 10;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    QJsonArray  badCoordinateArray;
    badCoordinateArray << -10.0 << -20.0 ;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    QJsonArray badCoordinateArray_second;
    badCoordinateArray_second << -10.0 << -20.0 << true;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray_second;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    QJsonArray  badCoordinateArray2;
    badCoordinateArray2 << 1 << 2;
    QJsonArray badCoordinateArray_third;
    badCoordinateArray_third << -10.0 << -20.0 << badCoordinateArray2;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray_third;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    // Test bad type

    badObject = jsonObject;
    badObject.remove("type");
    badObject["type"] = "foo";
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    qDebug() << errorString;

    // Test good load

    bool result = missionItem.load(jsonObject, _seq, errorString);
    if (!result) {
        qDebug() << errorString;
        QVERIFY(result);
    }
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testSimpleLoadFromJson(void)
{
    // We specifically test SimpleMissionItem loading as well since it has additional
    // signalling which can affect values.

    SimpleMissionItem simpleMissionItem(NULL);
    QString     errorString;
    QJsonArray  coordinateArray;
    QJsonObject jsonObject;

    coordinateArray << -10.0 << -20.0 <<-30.0;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    jsonObject.insert(MissionItem::_jsonCoordinateKey, coordinateArray);

    QJsonArray rgParams =  { 10, 20, 30, 40 };
    jsonObject.insert(MissionItem::_jsonParamsKey, rgParams);

    QVERIFY(simpleMissionItem.load(jsonObject, _seq, errorString));
    _checkExpectedMissionItem(simpleMissionItem.missionItem());
}

void MissionItemTest::_testSaveToJson(void)
{
    MissionItem missionItem;

    missionItem.setSequenceNumber(_seq);
    missionItem.setIsCurrentItem(true);
    missionItem.setFrame((MAV_FRAME)3);
    missionItem.setCommand((MAV_CMD)80);
    missionItem.setParam1(10.1234567);
    missionItem.setParam2(20.1234567);
    missionItem.setParam3(30.1234567);
    missionItem.setParam4(40.1234567);
    missionItem.setParam5(-10.1234567);
    missionItem.setParam6(-20.1234567);
    missionItem.setParam7(-30.1234567);
    missionItem.setAutoContinue(true);

    // Round trip item
    QJsonObject jsonObject;
    QString errorString;
    missionItem.save(jsonObject);
    QVERIFY(missionItem.load(jsonObject, _seq, errorString));

    QCOMPARE(missionItem.sequenceNumber(), _seq);
    QCOMPARE(missionItem.isCurrentItem(), false);
    QCOMPARE(missionItem.frame(), (MAV_FRAME)3);
    QCOMPARE(missionItem.command(), (MAV_CMD)80);
    QCOMPARE(missionItem.param1(), 10.1234567);
    QCOMPARE(missionItem.param2(), 20.1234567);
    QCOMPARE(missionItem.param3(), 30.1234567);
    QCOMPARE(missionItem.param4(), 40.1234567);
    QCOMPARE(missionItem.param5(), -10.1234567);
    QCOMPARE(missionItem.param6(), -20.1234567);
    QCOMPARE(missionItem.param7(), -30.1234567);
    QCOMPARE(missionItem.autoContinue(), true);
}
