#include "blefinder.h"

auto ControllerData = (QBluetoothUuid::CharacteristicType)0x8651;
auto Controller = (QBluetoothUuid::ServiceClassUuid)0x8650;

ConnectionHandler::ConnectionHandler(QObject *parent) : QObject(parent) {
  connect(&m_localDevice, &QBluetoothLocalDevice::hostModeStateChanged, this,
          &ConnectionHandler::hostModeChanged);
}

bool ConnectionHandler::alive() const {
  return m_localDevice.isValid() &&
         m_localDevice.hostMode() != QBluetoothLocalDevice::HostPoweredOff;
}

bool ConnectionHandler::requiresAddressType() const {
#if QT_CONFIG(bluez)
  return true;
#else
  return false;
#endif
}

QString ConnectionHandler::name() const { return m_localDevice.name(); }

QString ConnectionHandler::address() const {
  return m_localDevice.address().toString();
}

void ConnectionHandler::hostModeChanged(
    QBluetoothLocalDevice::HostMode /*mode*/) {
  emit deviceChanged();
}

/// DeviceInfo
DeviceInfo::DeviceInfo(const QBluetoothDeviceInfo &device) : m_device(device) {}

void DeviceInfo::setDevice(const QBluetoothDeviceInfo &device) {
  m_device = device;
  emit deviceChanged();
}

QString DeviceInfo::getName() const { return m_device.name(); }

QString DeviceInfo::getAddress() const { return m_device.address().toString(); }

QBluetoothDeviceInfo DeviceInfo::getDevice() const { return m_device; }

/// BluetoothBaseClass
BluetoothBaseClass::BluetoothBaseClass(QObject *parent) : QObject(parent) {}

QString BluetoothBaseClass::error() const { return m_error; }

void BluetoothBaseClass::setError(const QString &error) {
  if (m_error != error) {
    m_error = error;
    emit errorChanged();
  }
}

QString BluetoothBaseClass::info() const { return m_info; }
void BluetoothBaseClass::setInfo(const QString &info) {
  if (m_info != info) {
    m_info = info;
    emit infoChanged();
  }
}

void BluetoothBaseClass::clearMessages() {
  setInfo("");
  setError("");
}

/// BLEHandler
BLEHandler::BLEHandler(QObject *parent)
    : BluetoothBaseClass(parent), m_foundJoystickService(false) {
  qDebug("BLE handler created!");
}

void BLEHandler::setDevice(DeviceInfo *device) {
  clearMessages();
  m_currentDevice = device;

  // Disconnect and delete old connection
  if (m_control) {
    m_control->disconnectFromDevice();
    delete m_control;
    m_control = nullptr;
  }

  // Create new controller and connect it if device available
  if (m_currentDevice) {

    // Make connections
    //! [Connect-Signals-1]
    m_control =
        QLowEnergyController::createCentral(m_currentDevice->getDevice(), this);
    //! [Connect-Signals-1]
    m_control->setRemoteAddressType(m_addressType);
    //! [Connect-Signals-2]
    connect(m_control, &QLowEnergyController::serviceDiscovered, this,
            &BLEHandler::serviceDiscovered);
    connect(m_control, &QLowEnergyController::discoveryFinished, this,
            &BLEHandler::serviceScanDone);

    connect(m_control,
            static_cast<void (QLowEnergyController::*)(
                QLowEnergyController::Error)>(&QLowEnergyController::error),
            this, [this](QLowEnergyController::Error error) {
              Q_UNUSED(error);
              setError("Cannot connect to remote device.");
            });
    connect(m_control, &QLowEnergyController::connected, this, [this]() {
      setInfo("Controller connected. Search services...");
      m_control->discoverServices();
      emit aliveChanged();
    });
    connect(m_control, &QLowEnergyController::disconnected, this, [this]() {
      setError("LowEnergy controller disconnected");
      //            m_foundJoystickService = false;
      this->disconnectService();
      emit aliveChanged();
    });

    // Connect
    m_control->connectToDevice();
    //! [Connect-Signals-2]
  }
}

void BLEHandler::setAddressType(AddressType type) {
  switch (type) {
  case BLEHandler::AddressType::PublicAddress:
    m_addressType = QLowEnergyController::PublicAddress;
    break;
  case BLEHandler::AddressType::RandomAddress:
    m_addressType = QLowEnergyController::RandomAddress;
    break;
  }
}

BLEHandler::AddressType BLEHandler::addressType() const {
  if (m_addressType == QLowEnergyController::RandomAddress)
    return BLEHandler::AddressType::RandomAddress;

  return BLEHandler::AddressType::PublicAddress;
}

bool BLEHandler::alive() const {
  if (m_service)
    return m_service->state() == QLowEnergyService::ServiceDiscovered;
  return false;
}

void BLEHandler::disconnectService() {
  m_foundJoystickService = false;

  // disable notifications
  if (m_notificationDesc.isValid() && m_service &&
      m_notificationDesc.value() ==
          QByteArray::fromHex("0100")) { // TODO: remove magic values
    m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0000"));
  } else {
    if (m_control)
      m_control->disconnectFromDevice();

    delete m_service;
    m_service = nullptr;
  }
}

void BLEHandler::serviceDiscovered(const QBluetoothUuid &gatt) {
  if (gatt == QBluetoothUuid(Controller)) {
    qInfo("Bluetooth low energy service discovered");
    m_foundJoystickService = true;
  } else {
    qWarning("Failed to discover Bluetooth low energy service");
    //    m_foundJoystickService = false;
  }
}

void BLEHandler::serviceScanDone() {
  setInfo("Service scan done.");

  // Delete old service if available
  if (m_service) {
    delete m_service;
    m_service = nullptr;
  }

  if (m_foundJoystickService)
    m_service =
        m_control->createServiceObject(QBluetoothUuid(Controller), this);

  if (m_service) {
    connect(m_service, &QLowEnergyService::stateChanged, this,
            &BLEHandler::serviceStateChanged);
    connect(m_service, &QLowEnergyService::characteristicChanged, this,
            &BLEHandler::updateValues);
    connect(m_service, &QLowEnergyService::descriptorWritten, this,
            &BLEHandler::confirmedDescriptorWrite);
    m_service->discoverDetails();
  } else {
    setError("Controller Service not found.");
  }
}

void BLEHandler::serviceStateChanged(QLowEnergyService::ServiceState s) {
  emit aliveChanged();
  switch (s) {
  case QLowEnergyService::InvalidService:
    break;
  case QLowEnergyService::DiscoveryRequired:
    break;
  case QLowEnergyService::LocalService:
    break;
  case QLowEnergyService::DiscoveringServices:
    setInfo(tr("Discovering services..."));
    break;
  case QLowEnergyService::ServiceDiscovered: {
    setInfo(tr("Controller Service discovered."));

    const QLowEnergyCharacteristic hrChar =
        m_service->characteristic(QBluetoothUuid(ControllerData));
    if (!hrChar.isValid()) {
      setError("Controller Data not found.");
      break;
    }

    m_notificationDesc =
        hrChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
    if (m_notificationDesc.isValid())
      m_service->writeDescriptor(m_notificationDesc,
                                 QByteArray::fromHex("0100"));
    break;
  }
  }
}

void BLEHandler::updateValues(const QLowEnergyCharacteristic &c,
                              const QByteArray &value) {
  if (c.uuid() != QBluetoothUuid(ControllerData))
    return;
  values = value;
  emit newValues();
}

void BLEHandler::confirmedDescriptorWrite(const QLowEnergyDescriptor &d,
                                          const QByteArray &value) {
  if (d.isValid() && d == m_notificationDesc &&
      value == QByteArray::fromHex("0000")) {
    // disabled notifications -> assume disconnect intent
    m_control->disconnectFromDevice();
    delete m_service;
    m_service = nullptr;
  }
}

/// BLEFinder
BLEFinder::BLEFinder(QObject *parent) : BluetoothBaseClass(parent) {
  connectionHandler = new ConnectionHandler(this);
  m_deviceHandler = new BLEHandler(this);
  //! [devicediscovery-1]
  m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
  m_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(5000);

  connect(m_deviceDiscoveryAgent,
          &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this,
          &BLEFinder::addDevice);
  connect(m_deviceDiscoveryAgent,
          static_cast<void (QBluetoothDeviceDiscoveryAgent::*)(
              QBluetoothDeviceDiscoveryAgent::Error)>(
              &QBluetoothDeviceDiscoveryAgent::error),
          this, &BLEFinder::scanError);

  connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
          this, &BLEFinder::scanFinished);
  connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled,
          this, &BLEFinder::scanFinished);
  //! [devicediscovery-1]
}

BLEFinder::~BLEFinder() {
  qDeleteAll(m_devices);
  m_devices.clear();
  delete m_deviceHandler;
  delete connectionHandler;
}

BLEHandler *BLEFinder::handler(void) const { return m_deviceHandler; }

ConnectionHandler *BLEFinder::connhandler() const { return connectionHandler; }

bool BLEFinder::scanning() const { return m_deviceDiscoveryAgent->isActive(); }

QVariant BLEFinder::devices() { return QVariant::fromValue(m_devices); }

void BLEFinder::startSearch() {
  clearMessages();
  m_deviceHandler->setDevice(nullptr);
  qDeleteAll(m_devices);
  m_devices.clear();

  emit devicesChanged();

  //! [devicediscovery-2]
  m_deviceDiscoveryAgent->start(
      QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
  //! [devicediscovery-2]

  emit scanningChanged();
  setInfo(tr("Scanning for devices..."));
}

void BLEFinder::connectToService(const QString &address) {
  m_deviceDiscoveryAgent->stop();

  DeviceInfo *currentDevice = nullptr;
  for (QObject *entry : qAsConst(m_devices)) {
    auto device = qobject_cast<DeviceInfo *>(entry);
    if (device && device->getAddress() == address) {
      currentDevice = device;
      break;
    }
  }

  if (currentDevice)
    m_deviceHandler->setDevice(currentDevice);

  clearMessages();
}

void BLEFinder::addDevice(const QBluetoothDeviceInfo &device) {
  // If device is LowEnergy-device, add it to the list
  if (device.coreConfigurations() &
      QBluetoothDeviceInfo::LowEnergyCoreConfiguration) {
    m_devices.append(new DeviceInfo(device));
    setInfo(tr("Low Energy device found. Scanning more..."));
    //! [devicediscovery-3]
    emit devicesChanged();
    //! [devicediscovery-4]
  }
  //...
}

void BLEFinder::scanError(QBluetoothDeviceDiscoveryAgent::Error error) {
  if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
    setError(tr("The Bluetooth adaptor is powered off."));
  else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
    setError(tr("Writing or reading from the device resulted in an error."));
  else
    setError(tr("An unknown error has occurred."));
}

void BLEFinder::scanFinished() {
  if (m_devices.isEmpty())
    setError(tr("No Low Energy devices found."));
  else
    setInfo(tr("Scanning done."));

  emit scanningChanged();
  emit devicesChanged();
}
