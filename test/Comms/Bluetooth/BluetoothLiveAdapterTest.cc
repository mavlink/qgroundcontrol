#include "BluetoothLiveAdapterTest.h"

#include "BluetoothConfiguration.h"

#include <QtBluetooth/QBluetoothHostInfo>
#include <QtBluetooth/QBluetoothLocalDevice>
#include <QtCore/QElapsedTimer>
#include <QtCore/QRegularExpression>
#include <QtTest/QSignalSpy>

static QList<QBluetoothHostInfo> _localAdaptersOrSkip()
{
    return QBluetoothLocalDevice::allDevices();
}

static QVariantList _waitForConfigAdapters(BluetoothConfiguration &config, QSignalSpy &adapterStateSpy, int timeoutMs)
{
    QVariantList adapters = config.getAllAvailableAdapters();
    if (!adapters.isEmpty()) {
        return adapters;
    }

    // Block on the adapter-state signal in slices so the helper exits as soon
    // as adapters appear, yet still honours the overall timeout. Previously
    // the wait fell through to the next spy.wait slot on spurious wake-ups;
    // breaking out early when the list populates avoids wasted slices.
    QElapsedTimer waitTimer;
    waitTimer.start();

    while (adapters.isEmpty() && (waitTimer.elapsed() < timeoutMs)) {
        const int remaining = qMax(0, timeoutMs - static_cast<int>(waitTimer.elapsed()));
        (void) adapterStateSpy.wait(qMin(200, remaining));
        adapters = config.getAllAvailableAdapters();
    }

    return adapters;
}

static QString _firstAdapterAddressOrEmpty(const QVariantList &adapters)
{
    for (const QVariant &entry : std::as_const(adapters)) {
        const QString address = entry.toMap().value(QStringLiteral("address")).toString();
        if (!address.isEmpty()) {
            return address;
        }
    }

    return QString();
}

void BluetoothLiveAdapterTest::_testLiveAdapterSelectionAndState()
{
    // Qt's Bluetooth module may emit an uncategorized warning when BlueZ is not running (e.g. on headless CI).
    expectLogMessage(QtWarningMsg, QRegularExpression(QStringLiteral("Cannot find a compatible running Bluez")));

    const QList<QBluetoothHostInfo> localHosts = _localAdaptersOrSkip();
    if (localHosts.isEmpty()) {
        QSKIP("No local Bluetooth adapter detected on this host");
    }

    BluetoothConfiguration config("TestBT_liveAdapter");
    QSignalSpy adapterStateSpy(&config, &BluetoothConfiguration::adapterStateChanged);

    const QVariantList adapters = _waitForConfigAdapters(config, adapterStateSpy, 5000);

    QVERIFY2(!adapters.isEmpty(), "BluetoothConfiguration did not enumerate any adapters");

    const QString adapterAddress = _firstAdapterAddressOrEmpty(adapters);
    QVERIFY2(!adapterAddress.isEmpty(), "No valid adapter address exposed by BluetoothConfiguration");

    bool hostAddressSeen = false;
    for (const QBluetoothHostInfo &host : localHosts) {
        if (host.address().toString() == adapterAddress) {
            hostAddressSeen = true;
            break;
        }
    }
    QVERIFY2(hostAddressSeen, "Selected adapter address was not present in local Qt host list");

    config.selectAdapter(adapterAddress);

    QCOMPARE(config.getAdapterAddress(), adapterAddress);
    QVERIFY(config.isAdapterAvailable());
    QVERIFY(config.getHostMode() != QStringLiteral("Unavailable"));
}

void BluetoothLiveAdapterTest::_testLiveAdapterQueryApis()
{
    if (_localAdaptersOrSkip().isEmpty()) {
        QSKIP("No local Bluetooth adapter detected on this host");
    }

    BluetoothConfiguration config("TestBT_liveQueries");
    QSignalSpy adapterStateSpy(&config, &BluetoothConfiguration::adapterStateChanged);

    const QVariantList adapters = _waitForConfigAdapters(config, adapterStateSpy, 5000);
    QVERIFY2(!adapters.isEmpty(), "BluetoothConfiguration did not enumerate any adapters");

    const QString adapterAddress = _firstAdapterAddressOrEmpty(adapters);
    QVERIFY2(!adapterAddress.isEmpty(), "No valid adapter address exposed by BluetoothConfiguration");

    config.selectAdapter(adapterAddress);
    QCOMPARE(config.getAdapterAddress(), adapterAddress);

    const QVariantList connected = config.getConnectedDevices();
    for (const QVariant &entry : connected) {
        const QVariantMap device = entry.toMap();
        QVERIFY(device.contains(QStringLiteral("address")));
        QVERIFY(device.value(QStringLiteral("address")).toString().size() > 0);
        QVERIFY(device.contains(QStringLiteral("connected")));
        QCOMPARE(device.value(QStringLiteral("connected")).toBool(), true);
    }

    const QVariantList paired = config.getAllPairedDevices();
    for (const QVariant &entry : paired) {
        const QVariantMap device = entry.toMap();
        QVERIFY(device.contains(QStringLiteral("address")));
        QVERIFY(device.value(QStringLiteral("address")).toString().size() > 0);
        QVERIFY(device.contains(QStringLiteral("paired")));
        QCOMPARE(device.value(QStringLiteral("paired")).toBool(), true);
    }
}

void BluetoothLiveAdapterTest::_testLiveScanLifecycle()
{
    if (_localAdaptersOrSkip().isEmpty()) {
        QSKIP("No local Bluetooth adapter detected on this host");
    }

    BluetoothConfiguration config("TestBT_liveScan");
    QSignalSpy adapterStateSpy(&config, &BluetoothConfiguration::adapterStateChanged);
    QSignalSpy scanningSpy(&config, &BluetoothConfiguration::scanningChanged);
    QSignalSpy errorSpy(&config, &BluetoothConfiguration::errorOccurred);

    const QVariantList adapters = _waitForConfigAdapters(config, adapterStateSpy, 5000);
    QVERIFY2(!adapters.isEmpty(), "BluetoothConfiguration did not enumerate any adapters");

    const QString adapterAddress = _firstAdapterAddressOrEmpty(adapters);
    QVERIFY2(!adapterAddress.isEmpty(), "No valid adapter address exposed by BluetoothConfiguration");
    config.selectAdapter(adapterAddress);

    config.startScan();

    // Exit as soon as the config reports scanning=true OR an error is emitted,
    // rather than polling every 100 ms for up to 5 s. QTRY_VERIFY pumps the
    // event loop on a tight default interval (~50 ms) so signals land
    // promptly; the timeout matches the previous upper bound.
    QTRY_VERIFY_WITH_TIMEOUT(config.scanning() || (errorSpy.count() > 0), 5000);
    const bool observedScanOrError = config.scanning() || (errorSpy.count() > 0);
    QVERIFY2(observedScanOrError, "Scan neither started nor produced an error within timeout");

    if (!config.scanning() && (errorSpy.count() > 0)) {
        const QString reason = errorSpy.last().first().toString();
        const QByteArray skipMsg = QStringLiteral("Live scan unavailable on this host: %1").arg(reason).toUtf8();
        QSKIP(skipMsg.constData());
    }

    if (config.scanning()) {
        config.stopScan();
        // Single bounded QTRY call replaces the previous spy.wait / qWait(50)
        // fallback pair — exits immediately once scanning is false, otherwise
        // times out at the same 3 s bound.
        QTRY_VERIFY_WITH_TIMEOUT(!config.scanning(), 3000);
        QVERIFY2(!config.scanning(), "Scan did not stop within timeout");
    }
}

UT_REGISTER_TEST(BluetoothLiveAdapterTest, TestLabel::Integration, TestLabel::Comms)
