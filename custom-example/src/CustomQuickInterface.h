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
    Q_PROPERTY(bool     enableNewGimbalControls READ enableNewGimbalControls WRITE setEnableNewGimbalControls NOTIFY enableNewGimbalControlsChanged)
    Q_PROPERTY(bool     gimbalPitchInverted READ    gimbalPitchInverted WRITE setGimbalPitchInverted NOTIFY gimbalPitchInvertedChanged)
    Q_PROPERTY(bool     gimbalYawInverted   READ    gimbalYawInverted   WRITE setGimbalYawInverted   NOTIFY gimbalYawInvertedChanged)
    Q_PROPERTY(bool     gimbalPitchPidEnabled READ    gimbalPitchPidEnabled WRITE setGimbalPitchPidEnabled NOTIFY gimbalPitchPidEnabledChanged)
    Q_PROPERTY(bool     gimbalYawPidEnabled READ    gimbalYawPidEnabled WRITE setGimbalYawPidEnabled NOTIFY gimbalYawPidEnabledChanged)

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

    bool    enableNewGimbalControls           () { return _enableNewGimbalControls; }
    void    setEnableNewGimbalControls        (bool set);

    bool    gimbalPitchInverted           () { return _gimbalPitchInverted; }
    void    setGimbalPitchInverted        (bool set);
    bool    gimbalYawInverted           () { return _gimbalYawInverted; }
    void    setGimbalYawInverted        (bool set);

    bool    gimbalPitchPidEnabled           () { return _gimbalPitchPidEnabled; }
    void    setGimbalPitchPidEnabled        (bool set);
    bool    gimbalYawPidEnabled           () { return _gimbalYawPidEnabled; }
    void    setGimbalYawPidEnabled        (bool set);

signals:
    void    showGimbalControlChanged    ();
    void    showAttitudeWidgetChanged();
    void    showVirtualKeyboardChanged();
    void    useEmbeddedGimbalChanged();
    void    enableNewGimbalControlsChanged    ();
    void    gimbalPitchInvertedChanged    ();
    void    gimbalYawInvertedChanged    ();
    void    gimbalPitchPidEnabledChanged    ();
    void    gimbalYawPidEnabledChanged    ();

private:
    static bool _showGimbalControl;
    static bool _useEmbeddedGimbal;
    static bool _showAttitudeWidget;
    static bool _showVirtualKeyboard;
    static bool _enableNewGimbalControls;
    static bool _gimbalPitchInverted;
    static bool _gimbalYawInverted;
    static bool _gimbalPitchPidEnabled;
    static bool _gimbalYawPidEnabled;
    static std::unique_ptr<QSettings> _settings;
};
