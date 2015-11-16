/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef APMAirframeComponentController_H
#define APMAirframeComponentController_H

#include <QObject>
#include <QQuickItem>
#include <QList>
#include <QAbstractListModel>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"
#include "APMAirframeComponentAirframes.h"

class APMAirframeModel;

/// MVC Controller for APMAirframeComponent.qml.
class APMAirframeComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    APMAirframeComponentController(void);
    ~APMAirframeComponentController();
    
    Q_PROPERTY(bool showCustomConfigPanel MEMBER _showCustomConfigPanel NOTIFY showCustomConfigPanelChanged)
    Q_PROPERTY(APMAirframeModel* airframeTypesModel MEMBER _airframeTypesModel CONSTANT)
    Q_PROPERTY(QString currentAirframeType MEMBER _currentAirframeType CONSTANT)
    Q_PROPERTY(QString currentVehicleName MEMBER _currentVehicleName CONSTANT)
    Q_PROPERTY(int currentVehicleIndex MEMBER _currentVehicleIndex CONSTANT)
    Q_PROPERTY(int autostartId MEMBER _autostartId NOTIFY autostartIdChanged)
    Q_PROPERTY(QString fileParams MEMBER _currentFileParams)

    Q_INVOKABLE void changeAutostart(void);

    
    int currentAirframeIndex(void);
    void setCurrentAirframeIndex(int newIndex);
    
signals:
    void loadAirframesCompleted();
    void autostartIdChanged(int newAutostartId);
    void showCustomConfigPanelChanged(bool show);
    
private slots:
    void _waitParamWriteSignal(QVariant value);
    void _rebootAfterStackUnwind(void);
    void _fillAirFrames(void);
private:
    static bool _typesRegistered;
    QString         _currentAirframeType;
    QString         _currentVehicleName;
    int             _currentVehicleIndex;
    int             _autostartId;
    QString         _currentFileParams;
    bool            _showCustomConfigPanel;
    int             _waitParamWriteSignalCount;
    APMAirframeModel    *_airframeTypesModel;
};

class APMAirframe : public QObject
{
    Q_OBJECT
    
public:
    APMAirframe(const QString& name, const QString& paramsFile, int autostartId, QObject* parent = NULL);
    ~APMAirframe();
    
    Q_PROPERTY(QString text MEMBER _name CONSTANT)
    Q_PROPERTY(int autostartId MEMBER _autostartId CONSTANT)
    Q_PROPERTY(QString params MEMBER _paramsFile CONSTANT)
    
private:
    QString _name;
    QString _paramsFile;
    int     _autostartId;
};

class APMAirframeType : public QObject
{
    Q_OBJECT
    
public:
    APMAirframeType(const QString& name, const QString& imageResource, QObject* parent = NULL);
    ~APMAirframeType();
    
    Q_PROPERTY(QString name MEMBER _name CONSTANT)
    Q_PROPERTY(QString imageResource MEMBER _imageResource CONSTANT)
    Q_PROPERTY(QVariantList airframes MEMBER _airframes CONSTANT)
    
    void addAirframe(const QString& name, const QString& paramsFile, int autostartId);
    QString name() const;
    QString imageResource() const;

private:
    QString         _name;
    QString         _imageResource;
    QVariantList    _airframes;
};

class APMAirframeModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum APMAirFrameRoles {NAME = Qt::UserRole + 1, IMAGE, OBJECT, COLUMNS};
    QHash<int, QByteArray> roleNames() const;
    APMAirframeModel(QObject *parent);
    QVariant data(const QModelIndex &index, int role) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    void setAirframeTypes(const QList<APMAirframeType*>& airframeTypes);
    APMAirframeType *getAirframeType(const QString& airframeTypeName) const;
private:
     QList<APMAirframeType*> _airframeTypes;
};

#endif
