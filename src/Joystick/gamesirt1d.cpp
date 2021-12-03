#include "gamesirt1d.h"

GamesirT1d::GamesirT1d(const QString &name, BLEHandler *handler,
                       MultiVehicleManager *multiVehicleManager)
    : JoystickBLE(name, 6, 20, 1, handler, multiVehicleManager) {}

void GamesirT1d::updateValues() {
  auto *data = raw_values();

  if (data[0] == 0xc9) // Garbage values ahead
    return;

  buttonValues[0] = (bool)(data[9] & 0x01);    // A
  buttonValues[1] = (bool)(data[9] & 0x02);    // B
  buttonValues[2] = (bool)(data[9] & 0x08);    // X
  buttonValues[3] = (bool)(data[9] & 0x10);    // Y
  buttonValues[4] = (bool)(data[10] & 0x04);   // C1
  buttonValues[5] = (bool)(data[9] & 0x04);    // MENU
  buttonValues[6] = (bool)(data[10] & 0x08);   // C2
  buttonValues[7] = false;                     // Left joystick click
  buttonValues[8] = false;                     // Left joystick click
  buttonValues[9] = (bool)(data[9] & 0x40);    // L1
  buttonValues[10] = (bool)(data[9] & 0x80);   // R1
  buttonValues[11] = (bool)(data[11] == 0x01); // Up
  buttonValues[12] = (bool)(data[11] == 0x05); // Down
  buttonValues[13] = (bool)(data[11] == 0x07); // Left
  buttonValues[14] = (bool)(data[11] == 0x03); // Right

  // The raw range from joystick is 512 +/- 512
  // The expected analog range is 0 +/- INT16_MAX
  const float scale_512 = (float)INT16_MAX / 512;
  int mid = 512;

  int val = int((data[2] << 2) + (data[3] >> 6)); // LX
  axisValues[0] = scale_512 * (val - mid);
  val = int(((data[3] & 0x3f) << 4) + (data[4] >> 4)); // LY
  axisValues[1] = scale_512 * (val - mid);
  val = int(((data[4] & 0xf) << 6) + (data[5] >> 2)); // RX
  axisValues[2] = scale_512 * (val - mid);
  val = int(((data[5] & 0x3) << 8) + ((data[6]))); // RY
  axisValues[3] = scale_512 * (val - mid);

  const float scale_255 = (float)INT16_MAX / 255;
  val = (data[7]); // L2
  axisValues[4] = scale_255 * val;
  const int analog_trigger_val = 150; // A subjective value which feels good
  buttonValues[15] = val > analog_trigger_val;

  val = int(data[8]); // R2
  axisValues[5] = scale_255 * val;
  buttonValues[16] = val > analog_trigger_val;
}
