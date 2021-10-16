#pragma once
#include "joystickBLE.h"

class GamesirT1d : public JoystickBLE {
public:
  GamesirT1d(const QString &name, BLEHandler *handler,
             MultiVehicleManager *multiVehicleManager);
  virtual void updateValues() override;
};
