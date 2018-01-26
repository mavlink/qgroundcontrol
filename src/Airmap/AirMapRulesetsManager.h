/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "LifetimeChecker.h"
#include "AirspaceRulesetsProvider.h"
#include "AirMapSharedState.h"

#include <QGeoCoordinate>

#include <airmap/rulesets.h>

/**
 * @file AirMapRulesetsManager.h
 * Class to download rulesets from AirMap
 */

//-----------------------------------------------------------------------------
class AirMapRule : public AirspaceRule
{
    Q_OBJECT
    friend class AirMapRulesetsManager;
public:
    AirMapRule                  (QObject* parent = NULL);
    QString         id              () override { return _id; }
    QString         description     () override { return _description; }
    bool            isDefault       () override { return _isDefault; }
    QString         name            () override { return _name; }
    SelectionType   selectionType   () override { return _selectionType; }
private:
    QString         _id;
    QString         _description;
    bool            _isDefault;
    QString         _name;
    SelectionType   _selectionType;
};

//-----------------------------------------------------------------------------
class AirMapRulesetsManager : public AirspaceRulesetsProvider, public LifetimeChecker
{
    Q_OBJECT
public:
    AirMapRulesetsManager       (AirMapSharedState& shared);

    bool                valid       () override { return _valid; }
    QmlObjectListModel* rules       () override { return &_rules; }
    QString             defaultRule () override;
    int                 defaultIndex() override { return _defaultIndex; }
    int                 currentIndex() override { return _currentIndex; }
    void             setCurrentIndex(int index) override;

    void                setROI      (const QGeoCoordinate& center) override;

signals:
    void        error           (const QString& what, const QString& airmapdMessage, const QString& airmapdDetails);

private:
    enum class State {
        Idle,
        RetrieveItems,
    };
    int                             _defaultIndex;
    int                             _currentIndex;
    bool                            _valid;
    State                           _state = State::Idle;
    AirMapSharedState&              _shared;
    QmlObjectListModel              _rules;
};


