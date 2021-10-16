#pragma once

#include "Joystick.h"
#include "blefinder.h"
#include <QBitArray>
#include <QVector>

class JoystickBLE : public Joystick {
public:
  JoystickBLE(const QString &name, int axisCount, int buttonCount, int hatCount,
              BLEHandler *handler, MultiVehicleManager *multiVehicleManager);

  static QMap<QString, Joystick *>
  discover(MultiVehicleManager *_multiVehicleManager);
  static bool init(JoystickManager *manager, BLEFinder *finder);

  ~JoystickBLE();
  // This can be uncommented to hide the calibration buttons for
  // gamecontrollers in the future bool requiresCalibration(void) final {
  // return !_isGameController; }

protected:
  bool _open() override;
  void _close() override;
  bool _update() override;

  bool _getButton(int i) override;
  int _getAxis(int i) override;
  bool _getHat(int hat, int i) override;

  // Parse values recieved by BLEHandler, needs to run more frequently than
  // _update to compensate for garbage values
  virtual void updateValues() = 0;
  // Set the joystick state to neutral if it were to ex. disconnect
  void resetValues();

  BLEHandler *_handler;
  const quint8 *raw_values();

  QBitArray buttonValues;
  QVector<int> axisValues;
};
