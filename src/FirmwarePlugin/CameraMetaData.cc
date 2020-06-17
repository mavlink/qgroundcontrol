/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "CameraMetaData.h"

CameraMetaData::CameraMetaData(const QString&   name,
                               double           sensorWidth,
                               double           sensorHeight,
                               double           imageWidth,
                               double           imageHeight,
                               double           focalLength,
                               bool             landscape,
                               bool             fixedOrientation,
                               double           minTriggerInterval,
                               QObject*         parent)
    : QObject               (parent)
    , _name                 (name)
    , _sensorWidth          (sensorWidth)
    , _sensorHeight         (sensorHeight)
    , _imageWidth           (imageWidth)
    , _imageHeight          (imageHeight)
    , _focalLength          (focalLength)
    , _landscape            (landscape)
    , _fixedOrientation     (fixedOrientation)
    , _minTriggerInterval   (minTriggerInterval)
{

}
