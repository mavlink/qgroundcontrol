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

/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FACTBINDER_H
#define FACTBINDER_H

#include "Fact.h"
#include "AutoPilotPlugin.h"

#include <QObject>
#include <QString>

/// This object is used to instantiate a connection to a Fact from within Qml.
class FactBinder : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(int componentId READ componentId NOTIFY nameChanged)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(QVariant valueString READ valueString NOTIFY valueChanged)
    Q_PROPERTY(QString units READ units NOTIFY metaDataChanged)
	Q_PROPERTY(QVariant defaultValue READ defaultValue NOTIFY metaDataChanged)
    Q_PROPERTY(bool defaultValueAvailable READ defaultValueAvailable NOTIFY metaDataChanged)
    Q_PROPERTY(bool valueEqualsDefault READ valueEqualsDefault NOTIFY valueChanged)
	Q_PROPERTY(FactMetaData::ValueType_t type READ type  NOTIFY metaDataChanged)
	Q_PROPERTY(QString shortDescription READ shortDescription NOTIFY metaDataChanged)
	Q_PROPERTY(QString longDescription READ longDescription NOTIFY metaDataChanged)
	Q_PROPERTY(QVariant min READ min NOTIFY metaDataChanged)
	Q_PROPERTY(QVariant max READ max NOTIFY metaDataChanged)
	Q_PROPERTY(QString group READ group NOTIFY metaDataChanged)

public:
    FactBinder(void);
    
    int componentId(void) const;
    
    QString name(void) const;
    void setName(const QString& name);
    
    QVariant value(void) const;
    void setValue(const QVariant& value);
    
    QString valueString(void) const;
    
    QString                     units(void) const;
	QVariant                    defaultValue(void);
    bool                        defaultValueAvailable(void);
    bool                        valueEqualsDefault(void);
	FactMetaData::ValueType_t   type(void);
	QString                     shortDescription(void);
	QString                     longDescription(void);
	QVariant                    min(void);
	QVariant                    max(void);
	QString                     group(void);

signals:
    void factMissing(const QString& name);
    void nameChanged(void);
    void valueChanged(void);
	void metaDataChanged(void);
    
private slots:
    void _delayedFactMissing(void);
    
private:
    // Overrides from QObject
    void connectNotify(const QMetaMethod & signal);
    void disconnectNotify(const QMetaMethod & signal);
    
    AutoPilotPlugin*    _autopilotPlugin;
    Fact*               _fact;
    int                 _componentId;
    bool                _factMissingSignalConnected;
    QStringList         _missedFactMissingSignals;
};

#endif