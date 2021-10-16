#pragma once

#include <QObject>

#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothDeviceInfo>
#include <QTimer>
#include <QVariant>

#include <QLowEnergyController>
#include <QLowEnergyService>

#include <QBluetoothDeviceInfo>
#include <QBluetoothLocalDevice>

class ConnectionHandler : public QObject {
  Q_PROPERTY(bool alive READ alive NOTIFY deviceChanged)
  Q_PROPERTY(QString name READ name NOTIFY deviceChanged)
  Q_PROPERTY(QString address READ address NOTIFY deviceChanged)
  Q_PROPERTY(bool requiresAddressType READ requiresAddressType CONSTANT)

  Q_OBJECT
public:
  explicit ConnectionHandler(QObject *parent = nullptr);

  bool alive() const;
  bool requiresAddressType() const;
  QString name() const;
  QString address() const;

signals:
  void deviceChanged();

private slots:
  void hostModeChanged(QBluetoothLocalDevice::HostMode mode);

private:
  QBluetoothLocalDevice m_localDevice;
};

/// Class to store discovered bluetooth devices
class DeviceInfo : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString deviceName READ getName NOTIFY deviceChanged)
  Q_PROPERTY(QString deviceAddress READ getAddress NOTIFY deviceChanged)

public:
  DeviceInfo(const QBluetoothDeviceInfo &device);

  void setDevice(const QBluetoothDeviceInfo &device);
  QString getName() const;
  QString getAddress() const;
  QBluetoothDeviceInfo getDevice() const;

signals:
  void deviceChanged();

private:
  QBluetoothDeviceInfo m_device;
};

/// Base Class for bluetooth devices
class BluetoothBaseClass : public QObject {
  Q_OBJECT
  Q_PROPERTY(QString error READ error WRITE setError NOTIFY errorChanged)
  Q_PROPERTY(QString info READ info WRITE setInfo NOTIFY infoChanged)

public:
  explicit BluetoothBaseClass(QObject *parent = nullptr);

  QString error() const;
  void setError(const QString &error);

  QString info() const;
  void setInfo(const QString &info);

  void clearMessages();

signals:
  void errorChanged();
  void infoChanged();

private:
  QString m_error;
  QString m_info;
};

/// Bluetooth low energy device handler
class BLEHandler : public BluetoothBaseClass {
  Q_OBJECT

  Q_PROPERTY(bool alive READ alive NOTIFY aliveChanged)
  Q_PROPERTY(AddressType addressType READ addressType WRITE setAddressType)

  friend class JoystickBLE;

public:
  enum class AddressType { PublicAddress, RandomAddress };
  Q_ENUM(AddressType)

  BLEHandler(QObject *parent = nullptr);

  void setDevice(DeviceInfo *device);
  void setAddressType(AddressType type);
  AddressType addressType() const;

  bool alive() const;

signals:
  void aliveChanged();
  void newValues();

public slots:
  void disconnectService();

private:
  // QLowEnergyController
  void serviceDiscovered(const QBluetoothUuid &);
  void serviceScanDone();

  // QLowEnergyService
  void serviceStateChanged(QLowEnergyService::ServiceState s);
  void updateValues(const QLowEnergyCharacteristic &c, const QByteArray &value);
  void confirmedDescriptorWrite(const QLowEnergyDescriptor &d,
                                const QByteArray &value);

private:
  bool m_foundJoystickService;
  QByteArray values;
  QLowEnergyController *m_control = nullptr;
  QLowEnergyService *m_service = nullptr;
  QLowEnergyDescriptor m_notificationDesc;
  DeviceInfo *m_currentDevice = nullptr;

  QLowEnergyController::RemoteAddressType m_addressType =
      QLowEnergyController::PublicAddress;
};

/// Bluetooth low energy device finder
class BLEFinder : public BluetoothBaseClass {
  Q_OBJECT

  Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
  Q_PROPERTY(QVariant devices READ devices NOTIFY devicesChanged)
  Q_PROPERTY(BLEHandler *handler READ handler CONSTANT)
  Q_PROPERTY(ConnectionHandler *connhandler READ connhandler CONSTANT)

public:
  BLEFinder(QObject *parent = nullptr);
  ~BLEFinder();

  BLEHandler *handler(void) const;
  ConnectionHandler *connhandler() const;

  bool scanning() const;
  QVariant devices();

public slots:
  void startSearch();
  void connectToService(const QString &address);

private slots:
  void addDevice(const QBluetoothDeviceInfo &);
  void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
  void scanFinished();

signals:
  void scanningChanged();
  void devicesChanged();

private:
  BLEHandler *m_deviceHandler;
  QBluetoothDeviceDiscoveryAgent *m_deviceDiscoveryAgent;
  QList<QObject *> m_devices;
  ConnectionHandler *connectionHandler;
};
