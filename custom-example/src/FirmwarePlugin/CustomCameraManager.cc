/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Camera Controller
 *   @author Gus Grubba <gus@auterion.com>
 *
 */

#include "QGCApplication.h"
#include "CustomCameraManager.h"
#include "CustomCameraControl.h"

//-----------------------------------------------------------------------------
CustomCameraManager::CustomCameraManager(Vehicle *vehicle)
    : QGCCameraManager(vehicle)
{
}

//-----------------------------------------------------------------------------
void
CustomCameraManager::_thermalNextPalette()
{
    CustomCameraControl* pCamera = qobject_cast<CustomCameraControl*>(currentCameraInstance());
    if(pCamera) {
        qCDebug(CameraManagerLog) << "Switch to Next Palette";
        auto palettes = pCamera->irPalette();
        auto newIdx = palettes->enumIndex() + 1;
        if(newIdx >= palettes->enumValues().count() )
            newIdx = 0;
        palettes->setEnumIndex(newIdx);
    }
}
