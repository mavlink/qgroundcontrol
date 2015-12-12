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
class APMAirframeType;

/// MVC Controller for APMAirframeComponent.qml.
class APMAirframeComponentController : public FactPanelController
{
    Q_OBJECT
    
public:
    enum FrameId{FRAME_TYPE_PLUS = 0,
                 FRAME_TYPE_X = 1,
                 FRAME_TYPE_V = 2,
                 FRAME_TYPE_H = 3,
                 FRAME_TYPE_NEWY6 = 10};
    Q_ENUM(FrameId)

    APMAirframeComponentController(void);
    ~APMAirframeComponentController();
    
    Q_PROPERTY(QmlObjectListModel* airframeTypesModel MEMBER _airframeTypesModel CONSTANT)
    Q_PROPERTY(APMAirframeType* currentAirframeType READ currentAirframeType WRITE setCurrentAirframeType NOTIFY currentAirframeTypeChanged)
    Q_PROPERTY(APMAirframe* currentAirframe READ currentAirframe WRITE setCurrentAirframe NOTIFY currentAirframeChanged)

    int currentAirframeIndex(void);
    void setCurrentAirframeIndex(int newIndex);
    
signals:
    void loadAirframesCompleted();
    void currentAirframeTypeChanged(APMAirframeType* airframeType);
    void currentAirframeChanged(APMAirframe* airframe);

public slots:
    APMAirframeType *currentAirframeType() const;
    APMAirframe *currentAirframe() const;
    void setCurrentAirframeType(APMAirframeType *t);
    void setCurrentAirframe(APMAirframe *t);

private slots:
    void _fillAirFrames(void);
    void _finishVehicleSetup(void);
    void _factFrameChanged(QVariant v);

private:
    static bool _typesRegistered;
    APMAirframeType *_currentAirframeType;
    APMAirframe *_currentAirframe;
    int _waitParamWriteSignalCount;
    QmlObjectListModel *_airframeTypesModel;
};

class APMAirframe : public QObject
{
    Q_OBJECT
    
public:
    APMAirframe(const QString& name, const QString& paramsFile, int type, QObject* parent = NULL);
    ~APMAirframe();
    
    Q_PROPERTY(QString name MEMBER _name CONSTANT)
    Q_PROPERTY(int type MEMBER _type CONSTANT)
    Q_PROPERTY(QString params MEMBER _paramsFile CONSTANT)
    
    QString name() const;
    QString params() const;
    int type() const;

private:
    QString _name;
    QString _paramsFile;
    int _type;
};

class APMAirframeType : public QObject
{
    Q_OBJECT
    
public:
    APMAirframeType(const QString& name, const QString& imageResource, int type, QObject* parent = NULL);
    ~APMAirframeType();
    
    Q_PROPERTY(QString name MEMBER _name CONSTANT)
    Q_PROPERTY(QString imageResource MEMBER _imageResource CONSTANT)
    Q_PROPERTY(QVariantList airframes MEMBER _airframes CONSTANT)
    Q_PROPERTY(int type MEMBER _type CONSTANT)
    Q_PROPERTY(bool dirty MEMBER _dirty CONSTANT)
    void addAirframe(const QString& name, const QString& paramsFile, int type);

    QString name() const;
    QString imageResource() const;
    int type() const;
private:
    QString         _name;
    QString         _imageResource;
    QVariantList    _airframes;
    int _type;
    bool _dirty;
};

#endif
