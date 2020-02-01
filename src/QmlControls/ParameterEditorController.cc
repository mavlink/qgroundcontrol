/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "ParameterEditorController.h"
#include "QGCApplication.h"
#include "ParameterManager.h"
#include "SettingsManager.h"
#include "AppSettings.h"

#ifndef __mobile__
#include "QGCMapRCToParamDialog.h"
#endif

#include <QStandardPaths>

ParameterEditorController::ParameterEditorController(void)
    : _currentCategory          ("Standard")  // FIXME: firmware specific
    , _parameters               (new QmlObjectListModel(this))
    , _parameterMgr             (_vehicle->parameterManager())
    , _showModifiedOnly          (false)
{
    const QMap<QString, QMap<QString, QStringList> >& categoryMap = _parameterMgr->getComponentCategoryMap(_vehicle->defaultComponentId());
    _categories = categoryMap.keys();

    // Move default category to front
    _categories.removeOne(_currentCategory);
    _categories.prepend(_currentCategory);

    // There is a category for each non default component
    for (int compId: _parameterMgr->componentIds()) {
        if (compId != _vehicle->defaultComponentId()) {
            _categories.append(_parameterMgr->getComponentCategory(compId));
        }
    }

    // Be careful about no parameters
    if (categoryMap.contains(_currentCategory) && categoryMap[_currentCategory].size() != 0) {
        _currentGroup = categoryMap[_currentCategory].keys()[0];
    }

    _updateParameters();

    connect(this, &ParameterEditorController::searchTextChanged,        this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentCategoryChanged,   this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::currentGroupChanged,      this, &ParameterEditorController::_updateParameters);
    connect(this, &ParameterEditorController::showModifiedOnlyChanged,  this, &ParameterEditorController::_updateParameters);
}

ParameterEditorController::~ParameterEditorController()
{

}

QStringList ParameterEditorController::getGroupsForCategory(const QString& category)
{
    int compId = _parameterMgr->getComponentId(category);
    const QMap<QString, QMap<QString, QStringList> >& categoryMap = _parameterMgr->getComponentCategoryMap(compId);
    return categoryMap[category].keys();
}

QStringList ParameterEditorController::searchParameters(const QString& searchText, bool searchInName, bool searchInDescriptions)
{
    QStringList list;

    for(const QString &paramName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
        if (searchText.isEmpty()) {
            list += paramName;
        } else {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paramName);

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
        _parameterMgr->writeParametersToStream(stream);
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
        errors = _parameterMgr->readParametersFromStream(stream);
        file.close();

        if (!errors.isEmpty()) {
            emit showErrorMessage(errors);
        }
    }
}

void ParameterEditorController::refresh(void)
{
    _parameterMgr->refreshAllParameters();
}

void ParameterEditorController::resetAllToDefaults(void)
{
    _parameterMgr->resetAllParametersToDefaults();
    refresh();
}

void ParameterEditorController::resetAllToVehicleConfiguration(void)
{
    _parameterMgr->resetAllToVehicleConfiguration();
    refresh();
}

void ParameterEditorController::setRCToParam(const QString& paramName)
{
#ifdef __mobile__
    Q_UNUSED(paramName)
#else
    if (_uas) {
        Q_UNUSED(paramName)
        //-- TODO QGCMapRCToParamDialog * d = new QGCMapRCToParamDialog(paramName, _uas, qgcApp()->toolbox()->multiVehicleManager(), MainWindow::instance());
        //d->exec();
    }
#endif
}

bool ParameterEditorController::_shouldShow(Fact* fact)
{
    bool show = _showModifiedOnly ? (fact->defaultValueAvailable() ? (fact->valueEqualsDefault() ? false : true) : false) : true;
    return show;
}

void ParameterEditorController::_updateParameters(void)
{
    QObjectList newParameterList;
    QStringList searchItems = _searchText.split(' ', QString::SkipEmptyParts);

    if (searchItems.isEmpty() && !_showModifiedOnly) {
        int compId = _parameterMgr->getComponentId(_currentCategory);
        const QMap<QString, QMap<QString, QStringList> >& categoryMap = _parameterMgr->getComponentCategoryMap(compId);
        for (const QString& paramName: categoryMap[_currentCategory][_currentGroup]) {
            newParameterList.append(_parameterMgr->getParameter(compId, paramName));
        }
    } else {
        for(const QString &paraName: _parameterMgr->parameterNames(_vehicle->defaultComponentId())) {
            Fact* fact = _parameterMgr->getParameter(_vehicle->defaultComponentId(), paraName);
            bool matched = _shouldShow(fact);
            // All of the search items must match in order for the parameter to be added to the list
            if(matched) {
                for (const auto& searchItem : searchItems) {
                    if (!fact->name().contains(searchItem, Qt::CaseInsensitive) &&
                        !fact->shortDescription().contains(searchItem, Qt::CaseInsensitive) &&
                        !fact->longDescription().contains(searchItem, Qt::CaseInsensitive)) {
                        matched = false;
                    }
                }
            }
            if (matched) {
                newParameterList.append(fact);
            }
        }
    }

    _parameters->swapObjectList(newParameterList);
}
