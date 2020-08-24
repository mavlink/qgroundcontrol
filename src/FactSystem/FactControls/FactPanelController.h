/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#pragma once

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include <QObject>
#include <QQuickItem>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "QGCLoggingCategory.h"

Q_DECLARE_LOGGING_CATEGORY(FactPanelControllerLog)

/// FactPanelController is used for handling missing Facts from C++ code.
class FactPanelController : public QObject
{
    Q_OBJECT
public:
    FactPanelController();

    Q_PROPERTY(Vehicle* vehicle MEMBER _vehicle CONSTANT)

    Q_INVOKABLE Fact*   getParameterFact    (int componentId, const QString& name, bool reportMissing = true);
    Q_INVOKABLE bool    parameterExists     (int componentId, const QString& name);

    /// Queries the vehicle for parameters which were not available on initial download but should be available now.
    /// Signals missingParametersAvailable when done. Only works for MAV_COMP_ID_AUTOPILOT1 parameters.
    Q_INVOKABLE void    getMissingParameters(QStringList rgNames);

signals:
    void missingParametersAvailable(void);

protected:
    /// Checks for existence of the specified parameters
    /// @return true: all parameters exists, false: parameters missing and reported
    bool _allParametersExists(int componentId, QStringList names);

    /// Report a missing parameter
    void _reportMissingParameter(int componentId, const QString& name);

    Vehicle*            _vehicle    = nullptr;
    UASInterface*       _uas        = nullptr;
    AutoPilotPlugin*    _autopilot  = nullptr;

private slots:
    void _checkForMissingParameters(void);

private:
    QStringList _missingParameterWaitList;
    QTimer      _missingParametersTimer;
};
