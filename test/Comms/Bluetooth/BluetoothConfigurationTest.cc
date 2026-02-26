#include "BluetoothConfigurationTest.h"

#include "BluetoothConfiguration.h"
#include "LinkConfiguration.h"
#include "LinkManager.h"

#include <QtBluetooth/QBluetoothAddress>
#include <QtBluetooth/QBluetoothDeviceInfo>
#include <QtCore/QMetaProperty>
#include <QtCore/QSettings>
#include <QtTest/QSignalSpy>
#include <memory>

void BluetoothConfigurationTest::_testConstruction()
{
    BluetoothConfiguration config("TestBT_construct");

    QCOMPARE(config.type(), LinkConfiguration::TypeBluetooth);
    QCOMPARE(config.mode(), BluetoothConfiguration::BluetoothMode::ModeClassic);
    QVERIFY(!config.scanning());
}

void BluetoothConfigurationTest::_testCopyConstruction()
{
    BluetoothConfiguration original("TestBT_copyOrig");
    original.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

    const QString testUuid = QStringLiteral("{6e400001-b5a3-f393-e0a9-e50e24dcca9e}");
    original.setServiceUuid(testUuid);

    BluetoothConfiguration copy(&original);

    QCOMPARE(copy.mode(), BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
    QCOMPARE(copy.serviceUuid(), testUuid);
}

void BluetoothConfigurationTest::_testFactoryCreateAndDuplicate()
{
    std::unique_ptr<LinkConfiguration> created(
        LinkConfiguration::createSettings(LinkConfiguration::TypeBluetooth, "TestBT_factoryCreate"));
    QVERIFY(created);
    QCOMPARE(created->type(), LinkConfiguration::TypeBluetooth);

    auto *createdBluetoothConfig = qobject_cast<BluetoothConfiguration *>(created.get());
    QVERIFY(createdBluetoothConfig);

    createdBluetoothConfig->setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
    createdBluetoothConfig->setServiceUuid(QStringLiteral("{0000ffe0-0000-1000-8000-00805f9b34fb}"));

    std::unique_ptr<LinkConfiguration> duplicate(LinkConfiguration::duplicateSettings(created.get()));
    QVERIFY(duplicate);
    QCOMPARE(duplicate->type(), LinkConfiguration::TypeBluetooth);

    auto *duplicateBluetoothConfig = qobject_cast<BluetoothConfiguration *>(duplicate.get());
    QVERIFY(duplicateBluetoothConfig);
    QCOMPARE(duplicateBluetoothConfig->mode(), createdBluetoothConfig->mode());
    QCOMPARE(duplicateBluetoothConfig->serviceUuid(), createdBluetoothConfig->serviceUuid());
}

void BluetoothConfigurationTest::_testModeGetSet()
{
    BluetoothConfiguration config("TestBT_mode");
    QSignalSpy spy(&config, &BluetoothConfiguration::modeChanged);

    QCOMPARE(config.mode(), BluetoothConfiguration::BluetoothMode::ModeClassic);

    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
    QCOMPARE(config.mode(), BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
    QCOMPARE(spy.count(), 1);

    // Setting same mode should not emit
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
    QCOMPARE(spy.count(), 1);
}

void BluetoothConfigurationTest::_testBleUuidGetSet()
{
    BluetoothConfiguration config("TestBT_uuid");
    QSignalSpy serviceSpy(&config, &BluetoothConfiguration::serviceUuidChanged);
    QSignalSpy readSpy(&config, &BluetoothConfiguration::readUuidChanged);
    QSignalSpy writeSpy(&config, &BluetoothConfiguration::writeUuidChanged);

    // Defaults are Nordic UART
    QVERIFY(!config.serviceUuid().isEmpty());
    QVERIFY(!config.readUuid().isEmpty());
    QVERIFY(!config.writeUuid().isEmpty());

    const QString newUuid = QStringLiteral("{0000ffe0-0000-1000-8000-00805f9b34fb}");
    config.setServiceUuid(newUuid);
    QCOMPARE(config.serviceUuid(), newUuid);
    QCOMPARE(serviceSpy.count(), 1);

    config.setReadUuid(newUuid);
    QCOMPARE(config.readUuid(), newUuid);
    QCOMPARE(readSpy.count(), 1);

    config.setWriteUuid(newUuid);
    QCOMPARE(config.writeUuid(), newUuid);
    QCOMPARE(writeSpy.count(), 1);
}

void BluetoothConfigurationTest::_testBluetoothAvailabilityConsistency()
{
    QCOMPARE(BluetoothConfiguration::isBluetoothAvailable(), LinkManager::isBluetoothAvailable());
}

void BluetoothConfigurationTest::_testSettingsRoundtrip()
{
    QSettings settings;
    const QString root = QStringLiteral("BluetoothConfigTest");

    {
        BluetoothConfiguration config("TestBT_save");
        config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

        const QString uuid = QStringLiteral("{0000ffe0-0000-1000-8000-00805f9b34fb}");
        config.setServiceUuid(uuid);

        config.saveSettings(settings, root);
    }

    {
        BluetoothConfiguration config("TestBT_load");
        config.loadSettings(settings, root);

        QCOMPARE(config.mode(), BluetoothConfiguration::BluetoothMode::ModeLowEnergy);
        QCOMPARE(config.serviceUuid(), QStringLiteral("{0000ffe0-0000-1000-8000-00805f9b34fb}"));
    }

    settings.remove(root);
}

void BluetoothConfigurationTest::_testSelectAdapterInvalidAddress()
{
    BluetoothConfiguration config("TestBT_invalid");
    QSignalSpy errorSpy(&config, &BluetoothConfiguration::errorOccurred);

    config.selectAdapter(QString());
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.first().first().toString().contains("Invalid"));

    errorSpy.clear();
    config.selectAdapter(QStringLiteral("not-a-bt-address"));
    QCOMPARE(errorSpy.count(), 1);
}

void BluetoothConfigurationTest::_testBleAddressRotationUpdatesSelectedDevice()
{
    qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");

    BluetoothConfiguration config("TestBT_bleRotate");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

    QSignalSpy deviceSpy(&config, &BluetoothConfiguration::deviceChanged);
    QSignalSpy selectedRssiSpy(&config, &BluetoothConfiguration::selectedRssiChanged);

    const QBluetoothAddress address1(QStringLiteral("11:22:33:44:55:66"));
    const QBluetoothAddress address2(QStringLiteral("AA:BB:CC:DD:EE:FF"));
    QVERIFY(!address1.isNull());
    QVERIFY(!address2.isNull());

    QBluetoothDeviceInfo firstInfo(address1, QStringLiteral("BLEDevice"), 0);
    firstInfo.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
    firstInfo.setRssi(-70);

    QBluetoothDeviceInfo rotatedInfo(address2, QStringLiteral("BLEDevice"), 0);
    rotatedInfo.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
    rotatedInfo.setRssi(-60);

    QVERIFY(QMetaObject::invokeMethod(&config, "_deviceDiscovered", Qt::DirectConnection,
                                      Q_ARG(QBluetoothDeviceInfo, firstInfo)));

    QVariantList model = config.devicesModel();
    QCOMPARE(model.size(), 1);
    QCOMPARE(model.first().toMap().value("address").toString(), address1.toString());

    config.setDeviceByAddress(address1.toString());
    QCOMPARE(config.address(), address1.toString());

    deviceSpy.clear();
    selectedRssiSpy.clear();

    QVERIFY(QMetaObject::invokeMethod(&config, "_deviceDiscovered", Qt::DirectConnection,
                                      Q_ARG(QBluetoothDeviceInfo, rotatedInfo)));

    model = config.devicesModel();
    QCOMPARE(model.size(), 1);
    QCOMPARE(model.first().toMap().value("address").toString(), address2.toString());
    QCOMPARE(config.address(), address2.toString());
    QCOMPARE(deviceSpy.count(), 1);
    QCOMPARE(selectedRssiSpy.count(), 1);
}

void BluetoothConfigurationTest::_testClassicDiscoveryWithZeroRssiIsListed()
{
    qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");

    BluetoothConfiguration config("TestBT_classicZeroRssi");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeClassic);

    QSignalSpy modelSpy(&config, &BluetoothConfiguration::devicesModelChanged);

    const QBluetoothAddress address(QStringLiteral("12:34:56:78:9A:BC"));
    QVERIFY(!address.isNull());

    QBluetoothDeviceInfo info(address, QString(), 0);
    info.setCoreConfigurations(QBluetoothDeviceInfo::BaseRateCoreConfiguration);
    info.setRssi(0);

    QVERIFY(QMetaObject::invokeMethod(&config, "_deviceDiscovered", Qt::DirectConnection,
                                      Q_ARG(QBluetoothDeviceInfo, info)));

    const QVariantList model = config.devicesModel();
    QCOMPARE(model.size(), 1);
    const QVariantMap first = model.first().toMap();
    QCOMPARE(first.value(QStringLiteral("address")).toString(), address.toString());
    QVERIFY(!first.contains(QStringLiteral("rssi")));
    QVERIFY(modelSpy.count() >= 1);
}

void BluetoothConfigurationTest::_testBleDiscoveryWithZeroRssiIsListed()
{
    qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");

    BluetoothConfiguration config("TestBT_bleZeroRssi");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

    QSignalSpy modelSpy(&config, &BluetoothConfiguration::devicesModelChanged);

    const QBluetoothAddress address(QStringLiteral("AA:BB:CC:DD:EE:01"));
    QVERIFY(!address.isNull());

    QBluetoothDeviceInfo info(address, QStringLiteral("BLEZero"), 0);
    info.setCoreConfigurations(QBluetoothDeviceInfo::LowEnergyCoreConfiguration);
    info.setRssi(0);

    QVERIFY(QMetaObject::invokeMethod(&config, "_deviceDiscovered", Qt::DirectConnection,
                                      Q_ARG(QBluetoothDeviceInfo, info)));

    const QVariantList model = config.devicesModel();
    QCOMPARE(model.size(), 1);
    const QVariantMap first = model.first().toMap();
    QCOMPARE(first.value(QStringLiteral("name")).toString(), QStringLiteral("BLEZero"));
    QCOMPARE(first.value(QStringLiteral("address")).toString(), address.toString());
    QVERIFY(modelSpy.count() >= 1);
}

void BluetoothConfigurationTest::_testBleDiscoveryWithUnknownCoreConfigIsListed()
{
    qRegisterMetaType<QBluetoothDeviceInfo>("QBluetoothDeviceInfo");

    BluetoothConfiguration config("TestBT_bleUnknownCore");
    config.setMode(BluetoothConfiguration::BluetoothMode::ModeLowEnergy);

    QSignalSpy modelSpy(&config, &BluetoothConfiguration::devicesModelChanged);

    const QBluetoothAddress address(QStringLiteral("AA:BB:CC:DD:EE:02"));
    QVERIFY(!address.isNull());

    QBluetoothDeviceInfo info(address, QStringLiteral("BLEUnknownCore"), 0);
    // Leave core configuration as unknown on purpose.
    info.setRssi(-65);

    QVERIFY(QMetaObject::invokeMethod(&config, "_deviceDiscovered", Qt::DirectConnection,
                                      Q_ARG(QBluetoothDeviceInfo, info)));

    const QVariantList model = config.devicesModel();
    QCOMPARE(model.size(), 1);
    const QVariantMap first = model.first().toMap();
    QCOMPARE(first.value(QStringLiteral("name")).toString(), QStringLiteral("BLEUnknownCore"));
    QCOMPARE(first.value(QStringLiteral("address")).toString(), address.toString());
    QVERIFY(modelSpy.count() >= 1);
}

void BluetoothConfigurationTest::_testAdapterDiscoverablePropertyMeta()
{
    BluetoothConfiguration config("TestBT_discoverableMeta");

    const QMetaObject *meta = config.metaObject();
    const int propertyIndex = meta->indexOfProperty("adapterDiscoverable");
    QVERIFY(propertyIndex >= 0);

    const QMetaProperty property = meta->property(propertyIndex);
    QVERIFY(property.isValid());
    QCOMPARE(QString::fromLatin1(property.typeName()), QStringLiteral("bool"));
    QVERIFY(property.hasNotifySignal());
    QCOMPARE(QString::fromLatin1(property.notifySignal().name()), QStringLiteral("adapterStateChanged"));
}

void BluetoothConfigurationTest::_testDestroyDuringAsyncRefresh()
{
    // Test that destruction with pending async operations doesn't crash
    {
        BluetoothConfiguration config("TestBT_destroy");
        config.getAllAvailableAdapters(); // triggers async refresh
        // config destroyed here with potential in-flight QtConcurrent task
    }
    QVERIFY2(true, "destruction during async refresh must not crash");
}

UT_REGISTER_TEST(BluetoothConfigurationTest, TestLabel::Unit, TestLabel::Comms)
