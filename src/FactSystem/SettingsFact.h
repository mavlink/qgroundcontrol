/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Fact.h"

/// @brief A SettingsFact is Fact which holds a QSettings value.
class SettingsFact : public Fact
{
    Q_OBJECT
    
public:
    SettingsFact(QObject* parent = NULL);
    SettingsFact(QString settingsGroup, FactMetaData* metaData, QObject* parent = NULL);
    SettingsFact(const SettingsFact& other, QObject* parent = NULL);

    const SettingsFact& operator=(const SettingsFact& other);

    Q_PROPERTY(bool visible MEMBER _visible CONSTANT)

    // Must be called before any references to fact
    void setVisible(bool visible) { _visible = visible; }

private slots:
    void _rawValueChanged(QVariant value);

private:
    QString _settingsGroup;
    bool    _visible;
};
