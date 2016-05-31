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

#ifndef PARAMETEREDITORCONTROLLER_H
#define PARAMETEREDITORCONTROLLER_H

#include <QObject>
#include <QList>

#include "AutoPilotPlugin.h"
#include "UASInterface.h"
#include "FactPanelController.h"

class ParameterEditorController : public FactPanelController
{
    Q_OBJECT
    
public:
    ParameterEditorController(void);
    ~ParameterEditorController();

    Q_PROPERTY(QStringList componentIds MEMBER _componentIds CONSTANT)
	
	Q_INVOKABLE QStringList getGroupsForComponent(int componentId);
	Q_INVOKABLE QStringList getParametersForGroup(int componentId, QString group);
    Q_INVOKABLE QStringList searchParametersForComponent(int componentId, const QString& searchText, bool searchInName=true, bool searchInDescriptions=true);
	
	Q_INVOKABLE void clearRCToParam(void);
    Q_INVOKABLE void saveToFilePicker(void);
    Q_INVOKABLE void loadFromFilePicker(void);
    Q_INVOKABLE void saveToFile(const QString& filename);
    Q_INVOKABLE void loadFromFile(const QString& filename);
    Q_INVOKABLE void refresh(void);
    Q_INVOKABLE void resetAllToDefaults(void);
	Q_INVOKABLE void setRCToParam(const QString& paramName);
	
	QList<QObject*> model(void);
    
signals:
    void showErrorMessage(const QString& errorMsg);
	
private:
	QStringList			_componentIds;
};

#endif
