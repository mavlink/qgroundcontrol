#include "joystickBLE.h"
#include "JoystickManager.h"
#include "gamesirt1d.h"

QGC_LOGGING_CATEGORY(JoystickBLELog, "JoystickBLELog")

JoystickBLE::JoystickBLE(const QString &name, int axisCount, int buttonCount,
                         int hatCount, BLEHandler *handler,
                         MultiVehicleManager *multiVehicleManager)
    : Joystick(name, axisCount, buttonCount, hatCount, multiVehicleManager),
      _handler(handler) {
  buttonValues = QBitArray(buttonCount, false);
  axisValues = decltype(axisValues)(axisCount, 0);
  connect(handler, &BLEHandler::newValues, this, &JoystickBLE::updateValues);
  _rgAxisValues = new int[axisCount];
}

JoystickBLE::~JoystickBLE() {
  disconnect(_handler, &BLEHandler::newValues, this,
             &JoystickBLE::updateValues);
}

static JoystickManager *_manager = nullptr;
static BLEFinder *_device_finder = nullptr;

static void update_joystick(void) {
  if (_manager != nullptr) {
    qCDebug(JoystickBLELog) << "Sending updateJoystick signal!";
    emit _manager->updateAvailableJoysticksSignal();
  }
}

bool JoystickBLE::init(JoystickManager *manager, BLEFinder *finder) {
  _manager = manager;
  _device_finder = finder;

  connect(finder->handler(), &BLEHandler::aliveChanged, &update_joystick);

  return true;
}

QMap<QString, Joystick *>
JoystickBLE::discover(MultiVehicleManager *_multiVehicleManager) {
  static QMap<QString, Joystick *> ret;
  QMap<QString, Joystick *> newRet;
  if (_device_finder == nullptr)
    return {};
  if (_device_finder->handler()->m_foundJoystickService) {
    auto name = _device_finder->handler()->m_currentDevice->getName();
    JoystickBLE *js = nullptr;
    //    if (name.contains("Gamesir-T1d")) { // Currently does not work on
    //    macos
    //      js =
    //          new GamesirT1d(name, _device_finder->handler(),
    //          _multiVehicleManager);

    //    } // else if (...)

    js = new GamesirT1d(name, _device_finder->handler(), _multiVehicleManager);

    if (js != nullptr)
      newRet[name] = js;
  }
  ret = newRet;
  return ret;
}

bool JoystickBLE::_open(void) { return true; }

void JoystickBLE::_close(void) {}

bool JoystickBLE::_update(void) {
  // When the joystick is disconnected, set controls to neutral
  if (!_handler->alive())
    resetValues();

  return true;
}

bool JoystickBLE::_getButton(int i) { return buttonValues[i]; }

int JoystickBLE::_getAxis(int i) {
  qCDebug(JoystickBLELog) << "Return value " << axisValues[i] << " for axis "
                          << i;
  return axisValues[i];
}

bool JoystickBLE::_getHat(int hat, int i) {
  Q_UNUSED(hat);
  Q_UNUSED(i);
  return false;
}

void JoystickBLE::resetValues() {
  for (int i = 0; i < buttonValues.size(); i++) {
    buttonValues[i] = false;
  }
  for (int i = 0; i < axisValues.size(); i++) {
    axisValues[i] = 0;
  }
}

const quint8 *JoystickBLE::raw_values() {
  return reinterpret_cast<const quint8 *>(_handler->values.constData());
}
