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

class QSettings;

//-----------------------------------------------------------------------------
// QtQuick Interface (UI)
class CustomQuickInterface : public QObject
{
    Q_OBJECT
public:
    CustomQuickInterface(QObject* parent = nullptr);
    ~CustomQuickInterface();
    Q_PROPERTY(bool     showGimbalControl   READ    showGimbalControl   WRITE setShowGimbalControl   NOTIFY showGimbalControlChanged)
    Q_PROPERTY(bool     useEmbeddedGimbal   READ    useEmbeddedGimbal   WRITE setUseEmbeddedGimbal   NOTIFY useEmbeddedGimbalChanged)
    Q_PROPERTY(bool     showAttitudeWidget  READ    showAttitudeWidget  WRITE setShowAttitudeWidget  NOTIFY showAttitudeWidgetChanged)
    Q_PROPERTY(bool     showVirtualKeyboard READ    showVirtualKeyboard WRITE setShowVirtualKeyboard NOTIFY showVirtualKeyboardChanged)

    static void initSettings();
    void    init                        ();

    bool    showGimbalControl           () { return _showGimbalControl; }
    void    setShowGimbalControl        (bool set);

    bool    useEmbeddedGimbal           () { return _useEmbeddedGimbal; }
    void    setUseEmbeddedGimbal        (bool set);

    bool    showAttitudeWidget      () { return _showAttitudeWidget; }
    void    setShowAttitudeWidget   (bool set);

    static bool showVirtualKeyboard      () { return _showVirtualKeyboard; }
    void    setShowVirtualKeyboard   (bool set);

signals:
    void    showGimbalControlChanged    ();
    void    showAttitudeWidgetChanged();
    void    showVirtualKeyboardChanged();
    void    useEmbeddedGimbalChanged();

private:
    static bool _showGimbalControl;
    static bool _useEmbeddedGimbal;
    static bool _showAttitudeWidget;
    static bool _showVirtualKeyboard;
    static std::unique_ptr<QSettings> _settings;
};
