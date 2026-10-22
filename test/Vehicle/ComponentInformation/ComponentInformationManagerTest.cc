#include "ComponentInformationManagerTest.h"

#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>
#include <QtStateMachine/QStateMachine>
#include <QtTest/QSignalSpy>

#include "CompInfoGeneral.h"
#include "CompInfoParam.h"
#include "ComponentInformationManager.h"
#include "UnitTest.h"
#include "Vehicle.h"

namespace {

void _requestCompleteCallback(void* callbackData)
{
    bool* called = static_cast<bool*>(callbackData);
    if (called) {
        *called = true;
    }
}

}  // namespace

void ComponentInformationManagerTest::_requestAllComponentInformationCompletes()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    QSignalSpy finishedSpy(manager, &QStateMachine::finished);
    QSignalSpy progressSpy(manager, &ComponentInformationManager::progressUpdate);
    QVERIFY(finishedSpy.isValid());
    QVERIFY(progressSpy.isValid());

    bool callbackCalled = false;
    manager->requestAllComponentInformation(_requestCompleteCallback, &callbackCalled);

    if (finishedSpy.count() == 0) {
        QVERIFY(finishedSpy.wait(TestTimeout::longMs()));
    }

    QVERIFY(callbackCalled);
    QVERIFY(!manager->inProgress());
    QVERIFY(manager->progress() >= 0.99f);
    QVERIFY(progressSpy.count() > 0);
}

void ComponentInformationManagerTest::_progressSignalsStayInRange()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    QList<float> progressValues;
    const QMetaObject::Connection progressConn =
        connect(manager, &ComponentInformationManager::progressUpdate, this,
                [&progressValues](float progress) { progressValues.append(progress); });

    bool callbackCalled = false;
    QSignalSpy finishedSpy(manager, &QStateMachine::finished);
    manager->requestAllComponentInformation(_requestCompleteCallback, &callbackCalled);

    if (finishedSpy.count() == 0) {
        QVERIFY(finishedSpy.wait(TestTimeout::longMs()));
    }

    disconnect(progressConn);

    QVERIFY(callbackCalled);
    QVERIFY2(!progressValues.isEmpty(), "No progress updates were emitted");

    float maxProgress = 0.0f;
    float minProgress = 1.0f;
    for (const float progress : progressValues) {
        QVERIFY2(progress >= 0.0f && progress <= 1.0f,
                 qPrintable(QStringLiteral("Progress out of range: %1").arg(progress)));
        maxProgress = qMax(maxProgress, progress);
        minProgress = qMin(minProgress, progress);
    }

    QVERIFY2(maxProgress >= 0.5f,
             qPrintable(QStringLiteral("Expected progress to move forward, got max=%1").arg(maxProgress)));
    QVERIFY2((maxProgress - minProgress) >= 0.2f,
             qPrintable(QStringLiteral("Expected meaningful progress delta, got min=%1 max=%2")
                            .arg(minProgress)
                            .arg(maxProgress)));
}

void ComponentInformationManagerTest::_requestCanRunMultipleTimes()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    for (int i = 0; i < 3; ++i) {
        bool callbackCalled = false;
        QSignalSpy finishedSpy(manager, &QStateMachine::finished);
        manager->requestAllComponentInformation(_requestCompleteCallback, &callbackCalled);

        if (finishedSpy.count() == 0) {
            QVERIFY(finishedSpy.wait(TestTimeout::longMs()));
        }

        QVERIFY2(callbackCalled,
                 qPrintable(QStringLiteral("Completion callback not called on iteration %1").arg(i + 1)));
        QVERIFY(!manager->inProgress());
        QVERIFY(manager->progress() >= 0.99f);
    }
}

void ComponentInformationManagerTest::_compInfoAccessorsReturnValidObjects()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    CompInfoGeneral* const general = manager->compInfoGeneral(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(general);

    CompInfoParam* const param = manager->compInfoParam(MAV_COMP_ID_AUTOPILOT1);
    QVERIFY(param);

    CompInfoParam* const unknownCompParam1 = manager->compInfoParam(42);
    CompInfoParam* const unknownCompParam2 = manager->compInfoParam(42);
    QVERIFY(unknownCompParam1);
    QCOMPARE(unknownCompParam1, unknownCompParam2);

    bool callbackCalled = false;
    QSignalSpy finishedSpy(manager, &QStateMachine::finished);
    manager->requestAllComponentInformation(_requestCompleteCallback, &callbackCalled);

    if (finishedSpy.count() == 0) {
        QVERIFY(finishedSpy.wait(TestTimeout::longMs()));
    }

    QVERIFY(callbackCalled);
    // Metadata population timing is exercised in dedicated request-flow tests.
    // This accessor test only verifies manager/object stability across requests.
}

void ComponentInformationManagerTest::_replacementJsonReplacesResolvedMetaData()
{
    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);
    CompInfoParam* const param = manager->compInfoParam(43);
    QVERIFY(param);

    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto writeMetaData = [&tempDir](const QString& fileName, const QByteArray& shortDesc) {
        const QString path = tempDir.filePath(fileName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return QString();
        }
        (void) file.write(R"({"version":1,"parameters":[{"name":"REPLACED_PARAM","type":"Float","shortDesc":")" +
                          shortDesc + R"("}]})");
        return path;
    };
    const QString firstPath = writeMetaData(QStringLiteral("first.json"), "First");
    const QString secondPath = writeMetaData(QStringLiteral("second.json"), "Second");
    QVERIFY(!firstPath.isEmpty());
    QVERIFY(!secondPath.isEmpty());

    param->setJson(firstPath);
    FactMetaData* const first =
        param->factMetaDataForName(QStringLiteral("REPLACED_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(first);
    QCOMPARE(first->shortDescription(), QStringLiteral("First"));

    param->setJson(secondPath);
    FactMetaData* const second =
        param->factMetaDataForName(QStringLiteral("REPLACED_PARAM"), FactMetaData::valueTypeFloat);
    QVERIFY(second);
    QCOMPARE(second->shortDescription(), QStringLiteral("Second"));
    // Facts created before the replacement still reference the original metadata
    QCOMPARE(first->shortDescription(), QStringLiteral("First"));
}

void ComponentInformationManagerTest::_requestCompletesForArduPilot()
{
    _disconnectMockLink();
    _connectMockLink(MAV_AUTOPILOT_ARDUPILOTMEGA);

    auto* manager = vehicle()->compInfoManager();
    QVERIFY(manager);

    bool callbackCalled = false;
    QSignalSpy finishedSpy(manager, &QStateMachine::finished);
    manager->requestAllComponentInformation(_requestCompleteCallback, &callbackCalled);

    if (finishedSpy.count() == 0) {
        QVERIFY(finishedSpy.wait(TestTimeout::longMs()));
    }

    QVERIFY(callbackCalled);
    QVERIFY(vehicle()->isInitialConnectComplete());
}

UT_REGISTER_TEST(ComponentInformationManagerTest, TestLabel::Integration, TestLabel::Vehicle)
