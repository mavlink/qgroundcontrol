/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "ParameterEditorController.h"
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"

#ifndef __mobile__
#include "QGCFileDialog.h"
#include "QGCMapRCToParamDialog.h"
#include "MainWindow.h"
#endif

#include <QStandardPaths>

/// @Brief Constructs a new ParameterEditorController Widget. This widget is used within the PX4VehicleConfig set of screens.
ParameterEditorController::ParameterEditorController(void)
    : _currentComponentId(_vehicle->defaultComponentId())
    , _parameters(new QmlObjectListModel(this))
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();
    foreach (int componentId, groupMap.keys()) {
        _componentIds += QString("%1").arg(componentId);
    }

    _currentGroup = groupMap[_currentComponentId].keys()[0];
    _updateParameters();

    connect(this, &ParameterEditorController::searchTextChanged, this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentComponentIdChanged, this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentGroupChanged, this, &ParameterEditorController::_updateParameters);
}

ParameterEditorController::~ParameterEditorController()
{
    
}

QStringList ParameterEditorController::getGroupsForComponent(int componentId)
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();

    return groupMap[componentId].keys();
}

QStringList ParameterEditorController::getParametersForGroup(int componentId, QString group)
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();

    return groupMap[componentId][group];
}

QStringList ParameterEditorController::searchParametersForComponent(int componentId, const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;
    
    foreach(const QString &paramName, _autopilot->parameterNames(componentId)) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _autopilot->getParameterFact(componentId, paramName);
            
            if (searchInName && fact->name().contains(searchText, Qt::CaseInsensitive)) {
                list += paramName;
            } else if (searchInDescriptions && (fact->shortDescription().contains(searchText, Qt::CaseInsensitive) || fact->longDescription().contains(searchText, Qt::CaseInsensitive))) {
                list += paramName;
            }
        }
    }
    list.sort();
    
    return list;
}

void ParameterEditorController::clearRCToParam(void)
{
    Q_ASSERT(_uas);
    _uas->unsetRCToParameterMap();
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!_autopilot) {
        qWarning() << "Internal error _autopilot==NULL";
        return;
    }

    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to create file: %1").arg(filename));
            return;
        }
        
        QTextStream stream(&file);
        _autopilot->writeParametersToStream(stream);
        file.close();
    }
}

void ParameterEditorController::saveToFilePicker(void)
{
#ifndef __mobile__
    QString fileName = QGCFileDialog::getSaveFileName(NULL,
                                                      "Save Parameters",
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      "Parameter Files (*.params)",
                                                      "params",
                                                      true);
    saveToFile(fileName);
#endif
}

void ParameterEditorController::loadFromFile(const QString& filename)
{
    QString errors;
    
    if (!_autopilot) {
        qWarning() << "Internal error _autopilot==NULL";
        return;
    }

    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to open file: %1").arg(filename));
            return;
        }
        
        QTextStream stream(&file);
        errors = _autopilot->readParametersFromStream(stream);
        file.close();
        
        if (!errors.isEmpty()) {
            emit showErrorMessage(errors);
        }
    }
}

void ParameterEditorController::loadFromFilePicker(void)
{
#ifndef __mobile__
    QString fileName = QGCFileDialog::getOpenFileName(NULL,
                                                      "Load Parameters",
                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                      "Parameter Files (*.params);;All Files (*)");
    loadFromFile(fileName);
#endif
}

void ParameterEditorController::refresh(void)
{
    _autopilot->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    _autopilot->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::setRCToParam(const QString& paramName)
{
#ifdef __mobile__
    Q_UNUSED(paramName)
#else
    Q_ASSERT(_uas);
    QGCMapRCToParamDialog * d = new QGCMapRCToParamDialog(paramName, _uas, qgcApp()->toolbox()->multiVehicleManager(), MainWindow::instance());
    d->exec();
#endif
}

void ParameterEditorController::_updateParameters(void)
{
    QObjectList newParameterList;

    if (_searchText.isEmpty()) {
        const QMap<int, QMap<QString, QStringList> >& groupMap = _autopilot->getGroupMap();
        foreach (const QString& parameter, groupMap[_currentComponentId][_currentGroup]) {
            newParameterList.append(_autopilot->getParameterFact(_currentComponentId, parameter));
        }
    } else {
        foreach(const QString &parameter, _autopilot->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _autopilot->getParameterFact(_vehicle->defaultComponentId(), parameter);
            if (fact->name().contains(_searchText, Qt::CaseInsensitive) ||
                    fact->shortDescription().contains(_searchText, Qt::CaseInsensitive) ||
                    fact->longDescription().contains(_searchText, Qt::CaseInsensitive)) {
                newParameterList.append(fact);
            }
        }
    }

    _parameters->swapObjectList(newParameterList);
}
