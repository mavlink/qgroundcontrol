#include "InputCapabilities.h"

#include <QtGui/QInputDevice>

InputCapabilities::InputCapabilities(QObject* parent) : QObject(parent)
{
    refresh();
}

void InputCapabilities::refresh()
{
    bool touchDeviceAvailable = false;

    for (const auto* const inputDevice : QInputDevice::devices()) {
        if (inputDevice->type() == QInputDevice::DeviceType::TouchScreen) {
            touchDeviceAvailable = true;
            (void) connect(inputDevice, &QObject::destroyed, this, &InputCapabilities::refresh, Qt::UniqueConnection);
        }
    }

    if (_touchDeviceAvailable != touchDeviceAvailable) {
        _touchDeviceAvailable = touchDeviceAvailable;
        emit touchDeviceAvailableChanged();
    }
}
