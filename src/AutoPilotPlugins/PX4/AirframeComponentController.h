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

#ifndef AIRFRAMECOMPONENTCONTROLLER_H
#define AIRFRAMECOMPONENTCONTROLLER_H

#include <QObject>
#include <QQuickItem>
#include <QList>

#include "UASInterface.h"
#include "AutoPilotPlugin.h"
#include "FactPanelController.h"

/// MVC Controller for AirframeComponent.qml.
class AirframeComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    AirframeComponentController(void);
    ~AirframeComponentController();
    
    Q_PROPERTY(bool showCustomConfigPanel MEMBER _showCustomConfigPanel NOTIFY showCustomConfigPanelChanged)
    
    Q_PROPERTY(QVariantList airframeTypes MEMBER _airframeTypes CONSTANT)
    
    Q_PROPERTY(QString currentAirframeType MEMBER _currentAirframeType CONSTANT)
    Q_PROPERTY(QString currentVehicleName MEMBER _currentVehicleName CONSTANT)
    Q_PROPERTY(int currentVehicleIndex MEMBER _currentVehicleIndex CONSTANT)
    
    Q_PROPERTY(int autostartId MEMBER _autostartId NOTIFY autostartIdChanged)
    
    Q_INVOKABLE void changeAutostart(void);
    
    int currentAirframeIndex(void);
    void setCurrentAirframeIndex(int newIndex);
    
signals:
    void autostartIdChanged(int newAutostartId);
    void showCustomConfigPanelChanged(bool show);
    
private slots:
    void _waitParamWriteSignal(QVariant value);
    void _rebootAfterStackUnwind(void);
    
private:
    static bool _typesRegistered;
    
    QVariantList    _airframeTypes;
    QString         _currentAirframeType;
    QString         _currentVehicleName;
    int             _currentVehicleIndex;
    int             _autostartId;
    bool            _showCustomConfigPanel;
    int             _waitParamWriteSignalCount;
};

class Airframe : public QObject
{
    Q_OBJECT
    
public:
    Airframe(const QString& name, int autostartId, QObject* parent = nullptr);
    ~Airframe();
    
    Q_PROPERTY(QString text MEMBER _name CONSTANT)
    Q_PROPERTY(int autostartId MEMBER _autostartId CONSTANT)
    
private:
    QString _name;
    int     _autostartId;
};

class AirframeType : public QObject
{
    Q_OBJECT
    
public:
    AirframeType(const QString& name, const QString& imageResource, QObject* parent = nullptr);
    ~AirframeType();
    
    Q_PROPERTY(QString name MEMBER _name CONSTANT)
    Q_PROPERTY(QString imageResource MEMBER _imageResource CONSTANT)
    Q_PROPERTY(QVariantList airframes MEMBER _airframes CONSTANT)
    
    void addAirframe(const QString& name, int autostartId);
    
private:
    QString         _name;
    QString         _imageResource;
    QVariantList    _airframes;
};

#endif
