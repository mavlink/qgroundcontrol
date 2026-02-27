#include "MissionItemTest.h"

#include <QtCore/QJsonArray>
#include <QtTest/QSignalSpy>

#include "MissionItem.h"
#include "Fact.h"
#include "PlanMasterController.h"
#include "SimpleMissionItem.h"
#include "TestFixtures.h"

using namespace TestFixtures;

// Test property get/set
void MissionItemTest::_testSetGet()
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
void MissionItemTest::_testSignals()
{
    MissionItem missionItem(1,                                  // sequenceNumber
                            MAV_CMD_NAV_WAYPOINT,               // command
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                            1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  // params
                            true,                               // autoContinue
                            true);                              // isCurrentItem
    SignalSpyFixture missionItemSignals(&missionItem);
    QVERIFY(missionItemSignals.spy());
    // Validate isCurrentItemChanged signalling
    missionItemSignals.expectExactly("isCurrentItemChanged", 1);
    missionItem.setIsCurrentItem(true);
    QVERIFY(!missionItemSignals.wasEmitted("isCurrentItemChanged"));
    missionItem.setIsCurrentItem(false);
    QVERIFY(missionItemSignals.verify());
    QSignalSpy* spy = missionItemSignals.spy()->spy("isCurrentItemChanged");
    QList<QVariant> signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toBool(), false);
    missionItemSignals.clear();
    // Validate sequenceNumberChanged signalling
    missionItemSignals.expectExactly("sequenceNumberChanged", 1);
    missionItem.setSequenceNumber(1);
    QVERIFY(!missionItemSignals.wasEmitted("sequenceNumberChanged"));
    missionItem.setSequenceNumber(2);
    QVERIFY(missionItemSignals.verify());
    spy = missionItemSignals.spy()->spy("sequenceNumberChanged");
    signalArgs = spy->takeFirst();
    QCOMPARE(signalArgs.count(), 1);
    QCOMPARE(signalArgs[0].toInt(), 2);
}

// Test signalling associated with contained facts
void MissionItemTest::_testFactSignals()
{
    MissionItem missionItem(1,                                  // sequenceNumber
                            MAV_CMD_NAV_WAYPOINT,               // command
                            MAV_FRAME_GLOBAL_RELATIVE_ALT,      // MAV_FRAME
                            1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0,  // params
                            true,                               // autoContinue
                            true);                              // isCurrentItem
    // command
    QSignalSpy commandSpy(&missionItem._commandFact, &Fact::valueChanged);
    missionItem.setCommand(MAV_CMD_NAV_WAYPOINT);
    QCOMPARE(commandSpy.count(), 0);
    missionItem.setCommand(MAV_CMD_NAV_LAND);
    QCOMPARE(commandSpy.count(), 1);
    QList<QVariant> arguments = commandSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE((MAV_CMD)arguments.at(0).toInt(), MAV_CMD_NAV_LAND);
    // frame
    QSignalSpy frameSpy(&missionItem._frameFact, &Fact::valueChanged);
    missionItem.setFrame(MAV_FRAME_GLOBAL_RELATIVE_ALT);
    QCOMPARE(frameSpy.count(), 0);
    missionItem.setFrame(MAV_FRAME_BODY_NED);
    QCOMPARE(frameSpy.count(), 1);
    arguments = frameSpy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE((MAV_FRAME)arguments.at(0).toInt(), MAV_FRAME_BODY_NED);
    // param1
    QSignalSpy param1Spy(&missionItem._param1Fact, &Fact::valueChanged);
    missionItem.setParam1(1.0);
    QCOMPARE(param1Spy.count(), 0);
    missionItem.setParam1(2.0);
    QCOMPARE(param1Spy.count(), 1);
    arguments = param1Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 2.0);
    // param2
    QSignalSpy param2Spy(&missionItem._param2Fact, &Fact::valueChanged);
    missionItem.setParam2(2.0);
    QCOMPARE(param2Spy.count(), 0);
    missionItem.setParam2(3.0);
    QCOMPARE(param2Spy.count(), 1);
    arguments = param2Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 3.0);
    // param3
    QSignalSpy param3Spy(&missionItem._param3Fact, &Fact::valueChanged);
    missionItem.setParam3(3.0);
    QCOMPARE(param3Spy.count(), 0);
    missionItem.setParam3(4.0);
    QCOMPARE(param3Spy.count(), 1);
    arguments = param3Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 4.0);
    // param4
    QSignalSpy param4Spy(&missionItem._param4Fact, &Fact::valueChanged);
    missionItem.setParam4(4.0);
    QCOMPARE(param4Spy.count(), 0);
    missionItem.setParam4(5.0);
    QCOMPARE(param4Spy.count(), 1);
    arguments = param4Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 5.0);
    // param6
    QSignalSpy param6Spy(&missionItem._param6Fact, &Fact::valueChanged);
    missionItem.setParam6(6.0);
    QCOMPARE(param6Spy.count(), 0);
    missionItem.setParam6(7.0);
    QCOMPARE(param6Spy.count(), 1);
    arguments = param6Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 7.0);
    // param7
    QSignalSpy param7Spy(&missionItem._param7Fact, &Fact::valueChanged);
    missionItem.setParam7(7.0);
    QCOMPARE(param7Spy.count(), 0);
    missionItem.setParam7(8.0);
    QCOMPARE(param7Spy.count(), 1);
    arguments = param7Spy.takeFirst();
    QCOMPARE(arguments.count(), 1);
    QCOMPARE(arguments.at(0).toDouble(), 8.0);
}

void MissionItemTest::_checkExpectedMissionItem(const MissionItem& missionItem, bool allNaNs) const
{
    QCOMPARE(missionItem.sequenceNumber(), _seq);
    QCOMPARE(missionItem.isCurrentItem(), false);
    QCOMPARE(missionItem.frame(), (MAV_FRAME)3);
    QCOMPARE(missionItem.command(), (MAV_CMD)80);
    QCOMPARE(missionItem.autoContinue(), true);
    if (allNaNs) {
        QVERIFY(qIsNaN(missionItem.param1()));
        QVERIFY(qIsNaN(missionItem.param2()));
        QVERIFY(qIsNaN(missionItem.param3()));
        QVERIFY(qIsNaN(missionItem.param4()));
        QVERIFY(qIsNaN(missionItem.param5()));
        QVERIFY(qIsNaN(missionItem.param6()));
        QVERIFY(qIsNaN(missionItem.param7()));
    } else {
        QCOMPARE(missionItem.param1(), 10.0);
        QCOMPARE(missionItem.param2(), 20.0);
        QCOMPARE(missionItem.param3(), 30.0);
        QCOMPARE(missionItem.param4(), 40.0);
        QCOMPARE(missionItem.param5(), -10.0);
        QCOMPARE(missionItem.param6(), -20.0);
        QCOMPARE(missionItem.param7(), -30.0);
    }
}

void MissionItemTest::_testLoadFromStream()
{
    MissionItem missionItem;
    QString testString("10\t0\t3\t80\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n");
    QTextStream testStream(&testString, QIODevice::ReadOnly);
    QVERIFY(missionItem.load(testStream));
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testSimpleLoadFromStream()
{
    // We specifically test SimpleMissionItem loading as well since it has additional
    // signalling which can affect values.
    SimpleMissionItem simpleMissionItem(planController(), false /* flyView */, false /* forLoad */);
    QString testString("10\t0\t3\t80\t10\t20\t30\t40\t-10\t-20\t-30\t1\r\n");
    QTextStream testStream(&testString, QIODevice::ReadOnly);
    QVERIFY(simpleMissionItem.load(testStream));
    _checkExpectedMissionItem(simpleMissionItem.missionItem());
}

void MissionItemTest::_testLoadFromJsonV1()
{
    MissionItem missionItem;
    QString errorString;
    QJsonObject jsonObject = _createV1Json();
    // V1 format has param 1-4 in separate items instead of in params array
    QStringList removeKeys;
    removeKeys << MissionItem::_jsonParam1Key << MissionItem::_jsonParam2Key << MissionItem::_jsonParam3Key
               << MissionItem::_jsonParam4Key;
    for (const QString& removeKey : removeKeys) {
        QJsonObject badObject = jsonObject;
        badObject.remove(removeKey);
        QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
        QVERIFY(!errorString.isEmpty());
        TEST_DEBUG(errorString);
    }
    // Test good load
    bool success = missionItem.load(jsonObject, _seq, errorString);
    if (!success) {
        TEST_DEBUG(errorString);
    }
    QVERIFY(success);
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testLoadFromJsonV2()
{
    MissionItem missionItem;
    QString errorString;
    QJsonObject jsonObject = _createV2Json();
    // Test missing key detection
    QStringList removeKeys;
    removeKeys << MissionItem::_jsonCoordinateKey;
    for (const QString& removeKey : removeKeys) {
        QJsonObject badObject = jsonObject;
        badObject.remove(removeKey);
        QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
        QVERIFY(!errorString.isEmpty());
        TEST_DEBUG(errorString);
    }
    // Test bad coordinate variations
    QJsonObject badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = 10;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    TEST_DEBUG(errorString);
    QJsonArray badCoordinateArray;
    badCoordinateArray << -10.0 << -20.0;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    TEST_DEBUG(errorString);
    QJsonArray badCoordinateArray_second;
    badCoordinateArray_second << -10.0 << -20.0 << true;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray_second;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    TEST_DEBUG(errorString);
    QJsonArray badCoordinateArray2;
    badCoordinateArray2 << 1 << 2;
    QJsonArray badCoordinateArray_third;
    badCoordinateArray_third << -10.0 << -20.0 << badCoordinateArray2;
    badObject = jsonObject;
    badObject.remove("coordinate");
    badObject["coordinate"] = badCoordinateArray_third;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    TEST_DEBUG(errorString);
    // Test good load
    bool result = missionItem.load(jsonObject, _seq, errorString);
    if (!result) {
        TEST_DEBUG(errorString);
        QVERIFY(result);
    }
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testLoadFromJsonV3()
{
    MissionItem missionItem;
    QString errorString;
    QJsonObject jsonObject = _createV3Json();
    // Test missing key detection
    QStringList removeKeys;
    removeKeys << MissionItem::_jsonAutoContinueKey << MissionItem::_jsonCommandKey << MissionItem::_jsonFrameKey
               << MissionItem::_jsonParamsKey << VisualMissionItem::jsonTypeKey;
    for (const QString& removeKey : removeKeys) {
        QJsonObject badObject = jsonObject;
        badObject.remove(removeKey);
        QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
        QVERIFY(!errorString.isEmpty());
        TEST_DEBUG(errorString);
    }
    // Bad type
    QJsonObject badObject = jsonObject;
    badObject[VisualMissionItem::jsonTypeKey] = "foo";
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    TEST_DEBUG(errorString);
    // Incorrect param count
    badObject = jsonObject;
    QJsonArray rgParam = badObject[MissionItem::_jsonParamsKey].toArray();
    rgParam.removeFirst();
    badObject[MissionItem::_jsonParamsKey] = rgParam;
    QCOMPARE(missionItem.load(badObject, _seq, errorString), false);
    QVERIFY(!errorString.isEmpty());
    TEST_DEBUG(errorString);
    // Test good load
    bool result = missionItem.load(jsonObject, _seq, errorString);
    if (!result) {
        TEST_DEBUG(errorString);
        QVERIFY(result);
    }
    _checkExpectedMissionItem(missionItem);
}

void MissionItemTest::_testLoadFromJsonV3NaN()
{
    MissionItem missionItem;
    QString errorString;
    QJsonObject jsonObject = _createV3Json(true /* allNaNs */);
    bool result = missionItem.load(jsonObject, _seq, errorString);
    if (!result) {
        TEST_DEBUG(errorString);
        QVERIFY(result);
    }
    _checkExpectedMissionItem(missionItem, true /* allNaNs */);
}

void MissionItemTest::_testSimpleLoadFromJson()
{
    // We specifically test SimpleMissionItem loading as well since it has additional
    // signalling which can affect values.
    SimpleMissionItem simpleMissionItem(planController(), false /* flyView */, false /* forLoad */);
    QString errorString;
    QJsonArray coordinateArray;
    QJsonObject jsonObject;
    coordinateArray << -10.0 << -20.0 << -30.0;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    jsonObject.insert(MissionItem::_jsonCoordinateKey, coordinateArray);
    QJsonArray rgParams = {10, 20, 30, 40};
    jsonObject.insert(MissionItem::_jsonParamsKey, rgParams);
    QVERIFY(simpleMissionItem.load(jsonObject, _seq, errorString));
    _checkExpectedMissionItem(simpleMissionItem.missionItem());
}

void MissionItemTest::_testSaveToJson()
{
    MissionItem missionItem;
    QString errorString;
    QJsonObject jsonObject = _createV3Json(false /* allNaNs */);
    QVERIFY(missionItem.load(jsonObject, _seq, errorString));
    missionItem.save(jsonObject);
    QCOMPARE(jsonObject.contains(MissionItem::_jsonCoordinateKey), false);
    QVERIFY(missionItem.load(jsonObject, _seq, errorString));
    _checkExpectedMissionItem(missionItem, false /* allNaNs */);
    jsonObject = _createV3Json(true /* allNaNs */);
    QVERIFY(missionItem.load(jsonObject, _seq, errorString));
    missionItem.save(jsonObject);
    QVERIFY(missionItem.load(jsonObject, _seq, errorString));
    _checkExpectedMissionItem(missionItem, true /* allNaNs */);
}

QJsonObject MissionItemTest::_createV1Json()
{
    QJsonObject jsonObject;
    QJsonArray coordinateArray;
    coordinateArray << -10.0 << -20.0 << -30.0;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(MissionItem::_jsonParam1Key, 10);
    jsonObject.insert(MissionItem::_jsonParam2Key, 20);
    jsonObject.insert(MissionItem::_jsonParam3Key, 30);
    jsonObject.insert(MissionItem::_jsonParam4Key, 40);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    jsonObject.insert(MissionItem::_jsonCoordinateKey, coordinateArray);
    return jsonObject;
}

QJsonObject MissionItemTest::_createV2Json()
{
    QJsonObject jsonObject;
    QJsonArray coordinateArray;
    coordinateArray << -10.0 << -20.0 << -30.0;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    jsonObject.insert(MissionItem::_jsonCoordinateKey, coordinateArray);
    QJsonArray rgParams = {10, 20, 30, 40};
    jsonObject.insert(MissionItem::_jsonParamsKey, rgParams);
    return jsonObject;
}

QJsonObject MissionItemTest::_createV3Json(bool allNaNs)
{
    QJsonObject jsonObject;
    jsonObject.insert(MissionItem::_jsonAutoContinueKey, true);
    jsonObject.insert(MissionItem::_jsonCommandKey, 80);
    jsonObject.insert(MissionItem::_jsonFrameKey, 3);
    jsonObject.insert(VisualMissionItem::jsonTypeKey, VisualMissionItem::jsonTypeSimpleItemValue);
    if (allNaNs) {
        QJsonArray rgParams = {NAN, NAN, NAN, NAN, NAN, NAN, NAN};
        jsonObject.insert(MissionItem::_jsonParamsKey, rgParams);
    } else {
        QJsonArray rgParams = {10, 20, 30, 40, -10, -20, -30};
        jsonObject.insert(MissionItem::_jsonParamsKey, rgParams);
    }
    return jsonObject;
}

#include "UnitTest.h"

UT_REGISTER_TEST(MissionItemTest, TestLabel::Unit, TestLabel::MissionManager)
