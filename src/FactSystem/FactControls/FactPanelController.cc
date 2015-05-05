/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009 - 2014 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

#include "FactPanelController.h"
#include "UASManager.h"
#include "AutoPilotPluginManager.h"
#include "QGCMessageBox.h"

/// @file
///     @author Don Gagne <don@thegagnes.com>

FactPanelController::FactPanelController(void) :
	_autopilot(NULL),
    _factPanel(NULL)
{
    UASInterface* uas = UASManager::instance()->getActiveUAS();
    Q_ASSERT(uas);
    
    _autopilot = AutoPilotPluginManager::instance()->getInstanceForAutoPilotPlugin(uas);
    Q_ASSERT(_autopilot);
    Q_ASSERT(_autopilot->pluginReady());
    
    // Do a delayed check for the _factPanel finally being set correctly from Qml
    QTimer::singleShot(1000, this, &FactPanelController::_checkForMissingFactPanel);
}

QQuickItem* FactPanelController::factPanel(void)
{
    return _factPanel;
}

void FactPanelController::setFactPanel(QQuickItem* panel)
{
    // Once we finally have the _factPanel member set send any
    // missing fact notices that were waiting to go out
    
    _factPanel = panel;
    foreach (QString missingFact, _delayedMissingFacts) {
        _notifyPanelMissingFact(missingFact);
    }
    _delayedMissingFacts.clear();
}

void FactPanelController::_notifyPanelMissingFact(const QString& missingFact)
{
    QVariant returnedValue;

    QMetaObject::invokeMethod(_factPanel,
                              "showMissingFactOverlay",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, missingFact));
}

void FactPanelController::_reportMissingFact(const QString& missingFact)
{
    qgcApp()->reportMissingFact(missingFact);
    
    // If missing facts a reported from the constructor of a derived class we
    // will not have access to _factPanel yet. Just record list of missing facts
    // in that case instead of notify. Once _factPanel is available they will be
    // send out for real.
    if (_factPanel) {
        _notifyPanelMissingFact(missingFact);
    } else {
        _delayedMissingFacts += missingFact;
    }
}

bool FactPanelController::_allFactsExists(QStringList factList)
{
    bool noMissingFacts = true;
    
    foreach (QString fact, factList) {
        if (!_autopilot->parameterExists(fact)) {
            _reportMissingFact(fact);
            noMissingFacts = false;
        }
    }
    
    return noMissingFacts;
}

void FactPanelController::_checkForMissingFactPanel(void)
{
    if (!_factPanel) {
        QGCMessageBox::critical("Incorrect FactPanel Qml implementation", "FactPanelController used without passing in factPanel. This could lead to non-functioning user interface being displayed.");
    }
}