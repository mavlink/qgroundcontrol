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
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#ifndef __mobile__
#include "QGCQFileDialog.h"
#include "QGCMapRCToParamDialog.h"
#include "MainWindow.h"
#endif

#include <QStandardPaths>

/// @Brief Constructs a new ParameterEditorController Widget. This widget is used within the PX4VehicleConfig set of screens.
ParameterEditorController::ParameterEditorController(void)
    : _currentComponentId(_vehicle->defaultComponentId())
    , _parameters(new QmlObjectListModel(this))
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();
    foreach (int componentId, groupMap.keys()) {
        _componentIds += QString("%1").arg(componentId);
    }

    // Be careful about no parameters
    if (groupMap.contains(_currentComponentId) && groupMap[_currentComponentId].size() != 0) {
        _currentGroup = groupMap[_currentComponentId].keys()[0];
    }
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
    const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();

    return groupMap[componentId].keys();
}

QStringList ParameterEditorController::getParametersForGroup(int componentId, QString group)
{
    const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();

    return groupMap[componentId][group];
}

QStringList ParameterEditorController::searchParametersForComponent(int componentId, const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;
    
    foreach(const QString &paramName, _vehicle->parameterManager()->parameterNames(componentId)) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _vehicle->parameterManager()->getParameter(componentId, paramName);
            
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
    if (_uas) {
        _uas->unsetRCToParameterMap();
    }
}

void ParameterEditorController::saveToFile(const QString& filename)
{
    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to create file: %1").arg(filename));
            return;
        }
        
        QTextStream stream(&file);
        _vehicle->parameterManager()->writeParametersToStream(stream);
        file.close();
    }
}

void ParameterEditorController::loadFromFile(const QString& filename)
{
    QString errors;
    
    if (!filename.isEmpty()) {
        QFile file(filename);
        
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qgcApp()->showMessage(QString("Unable to open file: %1").arg(filename));
            return;
        }
        
        QTextStream stream(&file);
        errors = _vehicle->parameterManager()->readParametersFromStream(stream);
        file.close();
        
        if (!errors.isEmpty()) {
            emit showErrorMessage(errors);
        }
    }
}

void ParameterEditorController::refresh(void)
{
    _vehicle->parameterManager()->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    _vehicle->parameterManager()->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::setRCToParam(const QString& paramName)
{
#ifdef __mobile__
    Q_UNUSED(paramName)
#else
    if (_uas) {
        QGCMapRCToParamDialog * d = new QGCMapRCToParamDialog(paramName, _uas, qgcApp()->toolbox()->multiVehicleManager(), MainWindow::instance());
        d->exec();
    }
#endif
}

void ParameterEditorController::_updateParameters(void)
{
    QObjectList newParameterList;

    if (_searchText.isEmpty()) {
        const QMap<int, QMap<QString, QStringList> >& groupMap = _vehicle->parameterManager()->getGroupMap();
        foreach (const QString& parameter, groupMap[_currentComponentId][_currentGroup]) {
            newParameterList.append(_vehicle->parameterManager()->getParameter(_currentComponentId, parameter));
        }
    } else {
        foreach(const QString &parameter, _vehicle->parameterManager()->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), parameter);
            if (fact->name().contains(_searchText, Qt::CaseInsensitive) ||
                    fact->shortDescription().contains(_searchText, Qt::CaseInsensitive) ||
                    fact->longDescription().contains(_searchText, Qt::CaseInsensitive)) {
                newParameterList.append(fact);
            }
        }
    }

    _parameters->swapObjectList(newParameterList);
}
