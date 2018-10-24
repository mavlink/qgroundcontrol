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
    : _currentCategory("Standard")  // FIXME: firmware specific
    , _parameters(new QmlObjectListModel(this))
{
    const QMap<QString, QMap<QString, QStringList> >& categoryMap = _vehicle->parameterManager()->getCategoryMap();
    _categories = categoryMap.keys();

    // Move default category to front
    _categories.removeOne(_currentCategory);
    _categories.prepend(_currentCategory);

    // Be careful about no parameters
    if (categoryMap.contains(_currentCategory) && categoryMap[_currentCategory].size() != 0) {
        _currentGroup = categoryMap[_currentCategory].keys()[0];
    }
    _updateParameters();

    connect(this, &ParameterEditorController::searchTextChanged, this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentGroupChanged, this, &ParameterEditorController::_updateParameters);
}

ParameterEditorController::~ParameterEditorController()
{
    
}

QStringList ParameterEditorController::getGroupsForCategory(const QString& category)
{
    const QMap<QString, QMap<QString, QStringList> >& categoryMap = _vehicle->parameterManager()->getCategoryMap();

    return categoryMap[category].keys();
}

QStringList ParameterEditorController::getParametersForGroup(const QString& category, const QString& group)
{
    const QMap<QString, QMap<QString, QStringList> >& categoryMap = _vehicle->parameterManager()->getCategoryMap();

    return categoryMap[category][group];
}

QStringList ParameterEditorController::searchParameters(const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;
    
    for(const QString &paramName: _vehicle->parameterManager()->parameterNames(_vehicle->defaultComponentId())) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), paramName);
            
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
        QString parameterFilename = filename;
        if (!QFileInfo(filename).fileName().contains(".")) {
            parameterFilename += QString(".%1").arg(AppSettings::parameterFileExtension);
        }

        QFile file(parameterFilename);
        
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qgcApp()->showMessage(tr("Unable to create file: %1").arg(parameterFilename));
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
            qgcApp()->showMessage(tr("Unable to open file: %1").arg(filename));
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
    QStringList searchItems = _searchText.split(' ', QString::SkipEmptyParts);

    if (searchItems.isEmpty()) {
        const QMap<QString, QMap<QString, QStringList> >& categoryMap = _vehicle->parameterManager()->getCategoryMap();
        for (const QString& parameter: categoryMap[_currentCategory][_currentGroup]) {
            newParameterList.append(_vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), parameter));
        }
    } else {
        for(const QString &parameter: _vehicle->parameterManager()->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _vehicle->parameterManager()->getParameter(_vehicle->defaultComponentId(), parameter);
            bool matched = true;

            // all of the search items must match in order for the parameter to be added to the list
            for (const auto& searchItem : searchItems) {
                if (!fact->name().contains(searchItem, Qt::CaseInsensitive) &&
                        !fact->shortDescription().contains(searchItem, Qt::CaseInsensitive) &&
                        !fact->longDescription().contains(searchItem, Qt::CaseInsensitive)) {
                    matched = false;
                }
            }
            if (matched) {
                newParameterList.append(fact);
            }
        }
    }

    _parameters->swapObjectList(newParameterList);
}
