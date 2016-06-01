/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef FactPanelController_H
#define FactPanelController_H

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FactPanelControllerLog)

/// FactPanelController is used in combination with the FactPanel Qml control for handling
/// missing Facts from C++ code.
class FactPanelController : public QObject
{
    Q_OBJECT

public:
    FactPanelController(void);

    Q_PROPERTY(QQuickItem* factPanel READ factPanel WRITE setFactPanel)
    Q_PROPERTY(Vehicle* vehicle MEMBER _vehicle CONSTANT)

    Q_INVOKABLE Fact*   getParameterFact    (int componentId, const QString& name, bool reportMissing = true);
    Q_INVOKABLE bool    parameterExists     (int componentId, const QString& name);

    QQuickItem* factPanel(void);
    void setFactPanel(QQuickItem* panel);

protected:
    /// Checks for existence of the specified parameters
    /// @return true: all parameters exists, false: parameters missing and reported
    bool _allParametersExists(int componentId, QStringList names);

    /// Report a missing parameter to the FactPanel Qml element
    void _reportMissingParameter(int componentId, const QString& name);

    Vehicle*            _vehicle;
    UASInterface*       _uas;
    AutoPilotPlugin*    _autopilot;

private slots:
    void _checkForMissingFactPanel(void);

private:
    void _notifyPanelMissingParameter(const QString& missingParam);
    void _notifyPanelErrorMsg(const QString& errorMsg);
    void _showInternalError(const QString& errorMsg);

    QQuickItem*         _factPanel;
    QStringList         _delayedMissingParams;
};

#endif
