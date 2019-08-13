/****************************************************************************
 *
 * (c) 2009-2019 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 * @file
 *   @brief Custom QtQuick Interface
 *   @author Gus Grubba <gus@auterion.com>
 */

#pragma once

#include "Vehicle.h"

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QGeoPositionInfo>
#include <QGeoPositionInfoSource>

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class CustomQuickInterface : public QObject
{
    Q_OBJECT
public:
    CustomQuickInterface(QObject* parent = nullptr);
    ~CustomQuickInterface();
    Q_PROPERTY(bool     showGimbalControl   READ showGimbalControl  WRITE setShowGimbalControl  NOTIFY showGimbalControlChanged)
    Q_PROPERTY(bool         showAttitudeWidget  READ    showAttitudeWidget WRITE setShowAttitudeWidget NOTIFY showAttitudeWidgetChanged)

    bool    showGimbalControl           () { return _showGimbalControl; }
    void    setShowGimbalControl        (bool set);
    void    init                        ();

    bool    showAttitudeWidget      () { return _showAttitudeWidget; }
    void    setShowAttitudeWidget   (bool set);

signals:
    void    showGimbalControlChanged    ();
    void    showAttitudeWidgetChanged();

private:
    bool    _showGimbalControl  = true;
    bool _showAttitudeWidget = false;
};
