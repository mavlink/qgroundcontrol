/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "UnitTest.h"
#include "QGCApplication.h"
#include "MAVLinkProtocol.h"
#include "Vehicle.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "MockLink.h"
#include "LinkManager.h"

#include <QRandomGenerator>
#include <QTemporaryFile>
#include <QTime>

bool UnitTest::_messageBoxRespondedTo = false;
bool UnitTest::_badResponseButton = false;
QMessageBox::StandardButton UnitTest::_messageBoxResponseButton = QMessageBox::NoButton;
int UnitTest::_missedMessageBoxCount = 0;

bool UnitTest::_fileDialogRespondedTo = false;
bool UnitTest::_fileDialogResponseSet = false;
QStringList UnitTest::_fileDialogResponse;
enum UnitTest::FileDialogType UnitTest::_fileDialogExpectedType = getOpenFileName;
int UnitTest::_missedFileDialogCount = 0;

UnitTest::UnitTest(void)
{

}

UnitTest::~UnitTest()
{
    if (_unitTestRun) {
        // Derived classes must call base class implementations
        Q_ASSERT(_initCalled);
        Q_ASSERT(_cleanupCalled);
    }
}

void UnitTest::_addTest(UnitTest* test)
{
    QList<UnitTest*>& tests = _testList();

    Q_ASSERT(!tests.contains(test));

    tests.append(test);
}

void UnitTest::_unitTestCalled(void)
{
    _unitTestRun = true;
}

/// @brief Returns the list of unit tests.
QList<UnitTest*>& UnitTest::_testList(void)
{
    static QList<UnitTest*> tests;
    return tests;
}

int UnitTest::run(QString& singleTest)
{
    int ret = 0;

    for (UnitTest* test: _testList()) {
        if (singleTest.isEmpty() || singleTest == test->objectName()) {
            if (test->standalone() && singleTest.isEmpty()) {
                continue;
            }
            QStringList args;
            args << "*" << "-maxwarnings" << "0";
            ret += QTest::qExec(test, args);
        }
    }

    return ret;
}

/// @brief Called before each test.
///         Make sure to call first in your derived class
void UnitTest::init(void)
{
    _initCalled = true;

    if (!_linkManager) {
        _linkManager = qgcApp()->toolbox()->linkManager();
    }

    _linkManager->restart();

    // Force offline vehicle back to defaults
    AppSettings* appSettings = qgcApp()->toolbox()->settingsManager()->appSettings();
    appSettings->offlineEditingFirmwareClass()->setRawValue(appSettings->offlineEditingFirmwareClass()->rawDefaultValue());
    appSettings->offlineEditingVehicleClass()->setRawValue(appSettings->offlineEditingVehicleClass()->rawDefaultValue());

    _messageBoxRespondedTo = false;
    _missedMessageBoxCount = 0;
    _badResponseButton = false;
    _messageBoxResponseButton = QMessageBox::NoButton;

    _fileDialogRespondedTo = false;
    _missedFileDialogCount = 0;
    _fileDialogResponseSet = false;
    _fileDialogResponse.clear();

    _expectMissedFileDialog = false;
    _expectMissedMessageBox = false;

    MAVLinkProtocol::deleteTempLogFiles();
}

/// @brief Called after each test.
///         Make sure to call first in your derived class
void UnitTest::cleanup(void)
{
    _cleanupCalled = true;

    _disconnectMockLink();

    // Keep in mind that any code below these QCOMPARE may be skipped if the compare fails
    if (_expectMissedMessageBox) {
        QEXPECT_FAIL("", "Expecting failure due internal testing", Continue);
    }
    QCOMPARE(_missedMessageBoxCount, 0);
    if (_expectMissedFileDialog) {
        QEXPECT_FAIL("", "Expecting failure due internal testing", Continue);
    }
    QCOMPARE(_missedFileDialogCount, 0);

    // Don't let any lingering signals or events cross to the next unit test.
    // If you have a failure whose stack trace points to this then
    // your test is leaking signals or events. It could cause use-after-free or
    // segmentation faults from wild pointers.
    qgcApp()->processEvents();
}

void UnitTest::setExpectedMessageBox(QMessageBox::StandardButton response)
{
    //-- TODO
#if 0
    // This means that there was an expected message box but no call to checkExpectedMessageBox
    Q_ASSERT(!_messageBoxRespondedTo);
    
    Q_ASSERT(response != QMessageBox::NoButton);
    Q_ASSERT(_messageBoxResponseButton == QMessageBox::NoButton);
    
    // Make sure we haven't missed any previous message boxes
    int missedMessageBoxCount = _missedMessageBoxCount;
    QCOMPARE(missedMessageBoxCount, 0);
#endif
    _missedMessageBoxCount = 0;
    _messageBoxResponseButton = response;
}

void UnitTest::setExpectedFileDialog(enum FileDialogType type, QStringList response)
{
    //-- TODO
#if 0
    // This means that there was an expected file dialog but no call to checkExpectedFileDialog
    Q_ASSERT(!_fileDialogRespondedTo);
    
    // Multiple responses must be expected getOpenFileNames
    Q_ASSERT(response.count() <= 1 || type == getOpenFileNames);

    // Make sure we haven't missed any previous file dialogs
    int missedFileDialogCount = _missedFileDialogCount;
    _missedFileDialogCount = 0;
    QCOMPARE(missedFileDialogCount, 0);
#endif
    _fileDialogResponseSet = true;
    _fileDialogResponse = response;
    _fileDialogExpectedType = type;
}

void UnitTest::checkExpectedMessageBox(int expectFailFlags)
{
    // Previous call to setExpectedMessageBox should have already checked this
    Q_ASSERT(_missedMessageBoxCount == 0);

    // Check for a valid response

    if (expectFailFlags & expectFailBadResponseButton) {
        QEXPECT_FAIL("", "Expecting failure due to bad button response", Continue);
    }
    QCOMPARE(_badResponseButton, false);

    if (expectFailFlags & expectFailNoDialog) {
        QEXPECT_FAIL("", "Expecting failure due to no message box", Continue);
    }

    // Clear this flag before QCOMPARE since anything after QCOMPARE will be skipped on failure

    //-- TODO
    // bool messageBoxRespondedTo = _messageBoxRespondedTo;
    // QCOMPARE(messageBoxRespondedTo, true);
    _messageBoxRespondedTo = false;
}

void UnitTest::checkMultipleExpectedMessageBox(int messageCount)
{
    int missedMessageBoxCount = _missedMessageBoxCount;
    _missedMessageBoxCount = 0;
    QCOMPARE(missedMessageBoxCount, messageCount);
}

void UnitTest::checkExpectedFileDialog(int expectFailFlags)
{
    // Internal testing

    if (expectFailFlags & expectFailNoDialog) {
        QEXPECT_FAIL("", "Expecting failure due to no file dialog", Continue);
    }
    if (expectFailFlags & expectFailWrongFileDialog) {
        QEXPECT_FAIL("", "Expecting failure due to incorrect file dialog", Continue);
    } else {
        // Previous call to setExpectedFileDialog should have already checked this
        Q_ASSERT(_missedFileDialogCount == 0);
    }

    // Clear this flag before QCOMPARE since anything after QCOMPARE will be skipped on failure
    bool fileDialogRespondedTo = _fileDialogRespondedTo;
    _fileDialogRespondedTo = false;

    QCOMPARE(fileDialogRespondedTo, true);
}

QMessageBox::StandardButton UnitTest::_messageBox(QMessageBox::Icon icon, const QString& title, const QString& text, QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
{
    QMessageBox::StandardButton retButton;

    Q_UNUSED(icon);
    Q_UNUSED(title);
    Q_UNUSED(text);

    if (_messageBoxResponseButton == QMessageBox::NoButton) {
        // If no response button is set it means we were not expecting this message box. Response with default
        _missedMessageBoxCount++;
        retButton = defaultButton;
    } else {
        if (_messageBoxResponseButton & buttons) {
            // Everything is correct, use the specified response
            retButton = _messageBoxResponseButton;
        } else {
            // Trying to respond with a button not in the dialog. This is an error. Respond with default
            _badResponseButton = true;
            retButton = defaultButton;
        }
        _messageBoxRespondedTo = true;
    }

    // Clear response for next message box
    _messageBoxResponseButton = QMessageBox::NoButton;

    return retButton;
}

/// @brief Response to a file dialog which returns a single file
QString UnitTest::_fileDialogResponseSingle(enum FileDialogType type)
{
    QString retFile;

    if (!_fileDialogResponseSet || _fileDialogExpectedType != type) {
        // If no response is set or the type does not match what we expected it means we were not expecting this file dialog.
        // Respond with no selection.
        _missedFileDialogCount++;
    } else {
        Q_ASSERT(_fileDialogResponse.count() <= 1);
        if (_fileDialogResponse.count() == 1) {
            retFile = _fileDialogResponse[0];
        }
        _fileDialogRespondedTo = true;
    }

    // Clear response for next message box
    _fileDialogResponse.clear();
    _fileDialogResponseSet = false;

    return retFile;
}

QString UnitTest::_getExistingDirectory(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(options);

    return _fileDialogResponseSingle(getExistingDirectory);
}

QString UnitTest::_getOpenFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(filter);
    Q_UNUSED(options);

    return _fileDialogResponseSingle(getOpenFileName);
}

QStringList UnitTest::_getOpenFileNames(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(filter);
    Q_UNUSED(options);

    QStringList retFiles;

    if (!_fileDialogResponseSet || _fileDialogExpectedType != getOpenFileNames) {
        // If no response is set or the type does not match what we expected it means we were not expecting this file dialog.
        // Respond with no selection.
        _missedFileDialogCount++;
        retFiles.clear();
    } else {
        retFiles = _fileDialogResponse;
        _fileDialogRespondedTo = true;
    }

    // Clear response for next message box
    _fileDialogResponse.clear();
    _fileDialogResponseSet = false;

    return retFiles;
}

QString UnitTest::_getSaveFileName(
    QWidget* parent,
    const QString& caption,
    const QString& dir,
    const QString& filter,
    const QString& defaultSuffix,
    QFileDialog::Options options)
{
    Q_UNUSED(parent);
    Q_UNUSED(caption);
    Q_UNUSED(dir);
    Q_UNUSED(filter);
    Q_UNUSED(options);

    if(!defaultSuffix.isEmpty()) {
        Q_ASSERT(defaultSuffix.startsWith(".") == false);
    }

    return _fileDialogResponseSingle(getSaveFileName);
}

void UnitTest::_connectMockLink(MAV_AUTOPILOT autopilot, MockConfiguration::FailureMode_t failureMode)
{
    Q_ASSERT(!_mockLink);

    QSignalSpy spyVehicle(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged);

    switch (autopilot) {
    case MAV_AUTOPILOT_PX4:
        _mockLink = MockLink::startPX4MockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_ARDUPILOTMEGA:
        _mockLink = MockLink::startAPMArduCopterMockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_GENERIC:
        _mockLink = MockLink::startGenericMockLink(false, failureMode);
        break;
    case MAV_AUTOPILOT_INVALID:
        _mockLink = MockLink::startNoInitialConnectMockLink(false);
        break;
    default:
        qWarning() << "Type not supported";
        break;
    }

    // Wait for the Vehicle to get created
    QCOMPARE(spyVehicle.wait(10000), true);
    _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
    QVERIFY(_vehicle);

    if (autopilot != MAV_AUTOPILOT_INVALID) {
        // Wait for initial connect sequence to complete
        QSignalSpy spyPlan(_vehicle, &Vehicle::initialConnectComplete);
        QCOMPARE(spyPlan.wait(30000), true);
    }
}

void UnitTest::_disconnectMockLink(void)
{
    if (_mockLink) {
        QSignalSpy spyVehicle(qgcApp()->toolbox()->multiVehicleManager(), &MultiVehicleManager::activeVehicleChanged);

        _mockLink->disconnect();
        _mockLink = nullptr;

        // Wait for all the vehicle to go away
        QCOMPARE(spyVehicle.wait(10000), true);
        _vehicle = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle();
        QVERIFY(!_vehicle);
    }
}

void UnitTest::_linkDeleted(LinkInterface* link)
{
    if (link == _mockLink) {
        _mockLink = nullptr;
    }
}

QString UnitTest::createRandomFile(uint32_t byteCount)
{
    QTemporaryFile tempFile;

    QTime time = QTime::currentTime();
    QRandomGenerator::global()->seed(time.msec());

    tempFile.setAutoRemove(false);
    if (tempFile.open()) {
        for (uint32_t bytesWritten=0; bytesWritten<byteCount; bytesWritten++) {
            unsigned char byte = (QRandomGenerator::global()->generate() * 0xFF) / RAND_MAX;
            tempFile.write((char *)&byte, 1);
        }
        tempFile.close();
        return tempFile.fileName();
    } else {
        qWarning() << "UnitTest::createRandomFile open failed" << tempFile.errorString();
        return QString();
    }
}

bool UnitTest::fileCompare(const QString& file1, const QString& file2)
{
    QFile f1(file1);
    QFile f2(file2);

    if (QFileInfo(file1).size() != QFileInfo(file2).size()) {
        qWarning() << "UnitTest::fileCompare file sizes differ size1:size2" << QFileInfo(file1).size() << QFileInfo(file2).size();
        return false;
    }

    if (!f1.open(QIODevice::ReadOnly)) {
        qWarning() << "UnitTest::fileCompare unable to open file1:" << f1.errorString();
        return false;
    }
    if (!f2.open(QIODevice::ReadOnly)) {
        qWarning() << "UnitTest::fileCompare unable to open file1:" << f1.errorString();
        return false;
    }

    qint64 bytesRemaining = QFileInfo(file1).size();
    qint64 offset = 0;
    while (bytesRemaining) {
        uint8_t b1, b2;

        qint64 bytesRead = f1.read((char*)&b1, 1);
        if (bytesRead != 1) {
            qWarning() << "UnitTest::fileCompare file1 read failed:" << f1.errorString();
            return false;
        }
        bytesRead = f2.read((char*)&b2, 1);
        if (bytesRead != 1) {
            qWarning() << "UnitTest::fileCompare file2 read failed:" << f2.errorString();
            return false;
        }

        if (b1 != b2) {
            qWarning() << "UnitTest::fileCompare mismatch offset:b1:b2" << offset << b1 << b2;
            return false;
        }

        offset++;
        bytesRemaining--;
    }

    return true;
}

void UnitTest::changeFactValue(Fact* fact,double increment)
{
    if (fact->typeIsBool()) {
        fact->setRawValue(!fact->rawValue().toBool());
    } else {
        if (increment == 0) {
            increment = 1;
        }
        fact->setRawValue(fact->rawValue().toDouble() + increment);
    }
}

void UnitTest::_missionItemsEqual(MissionItem& actual, MissionItem& expected)
{
    QCOMPARE(static_cast<int>(actual.command()),    static_cast<int>(expected.command()));
    QCOMPARE(static_cast<int>(actual.frame()),      static_cast<int>(expected.frame()));
    QCOMPARE(actual.autoContinue(),                 expected.autoContinue());

    QVERIFY(QGC::fuzzyCompare(actual.param1(), expected.param1()));
    QVERIFY(QGC::fuzzyCompare(actual.param2(), expected.param2()));
    QVERIFY(QGC::fuzzyCompare(actual.param3(), expected.param3()));
    QVERIFY(QGC::fuzzyCompare(actual.param4(), expected.param4()));
    QVERIFY(QGC::fuzzyCompare(actual.param5(), expected.param5()));
    QVERIFY(QGC::fuzzyCompare(actual.param6(), expected.param6()));
    QVERIFY(QGC::fuzzyCompare(actual.param7(), expected.param7()));
}

bool UnitTest::fuzzyCompareLatLon(const QGeoCoordinate& coord1, const QGeoCoordinate& coord2)
{
    return coord1.distanceTo(coord2) < 1.0;
}

QGeoCoordinate UnitTest::changeCoordinateValue(const QGeoCoordinate& coordinate)
{
    return coordinate.atDistanceAndAzimuth(1, 0);
}
