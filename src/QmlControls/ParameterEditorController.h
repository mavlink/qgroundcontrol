/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef PARAMETEREDITORCONTROLLER_H
#define PARAMETEREDITORCONTROLLER_H

#include <QObject>
#include <QList>

#include "AutoPilotPlugin.h"
#include "UASInterface.h"
#include "FactPanelController.h"
#include "QmlObjectListModel.h"
#include "ParameterManager.h"

class ParameterEditorController : public FactPanelController
{
    Q_OBJECT

public:
    ParameterEditorController(void);
    ~ParameterEditorController();

    Q_PROPERTY(QString              searchText          MEMBER _searchText          NOTIFY searchTextChanged)
    Q_PROPERTY(QString              currentCategory     MEMBER _currentCategory     NOTIFY currentCategoryChanged)
    Q_PROPERTY(QString              currentGroup        MEMBER _currentGroup        NOTIFY currentGroupChanged)
    Q_PROPERTY(QmlObjectListModel*  parameters          MEMBER _parameters          CONSTANT)
    Q_PROPERTY(QStringList          categories          MEMBER _categories          CONSTANT)
    Q_PROPERTY(bool                 showModifiedOnly    MEMBER _showModifiedOnly    NOTIFY showModifiedOnlyChanged)

    Q_INVOKABLE QStringList getGroupsForCategory(const QString& category);
    Q_INVOKABLE QStringList searchParameters(const QString& searchText, bool searchInName=true, bool searchInDescriptions=true);

    Q_INVOKABLE void clearRCToParam(void);
    Q_INVOKABLE void saveToFile(const QString& filename);
    Q_INVOKABLE void loadFromFile(const QString& filename);
    Q_INVOKABLE void refresh(void);
    Q_INVOKABLE void resetAllToDefaults(void);
    Q_INVOKABLE void resetAllToVehicleConfiguration(void);
    Q_INVOKABLE void setRCToParam(const QString& paramName);

    QList<QObject*> model(void);

signals:
    void searchTextChanged(QString searchText);
    void currentCategoryChanged(QString category);
    void currentGroupChanged(QString group);
    void showErrorMessage(const QString& errorMsg);
    void showModifiedOnlyChanged();

private slots:
    void _updateParameters(void);

private:
    bool _shouldShow(Fact *fact);

private:
    QStringList         _categories;
    QString             _searchText;
    QString             _currentCategory;
    QString             _currentGroup;
    QmlObjectListModel* _parameters;
    ParameterManager*   _parameterMgr;
    bool                _showModifiedOnly;
};

#endif
