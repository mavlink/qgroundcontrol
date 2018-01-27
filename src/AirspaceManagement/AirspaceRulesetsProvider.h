/****************************************************************************
 *
 *   (c) 2017 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

//-----------------------------------------------------------------------------
/**
 * @class AirspaceRulesetsProvider
 * Base class that queries for airspace rulesets
 */

#include "QmlObjectListModel.h"

#include <QObject>
#include <QGeoCoordinate>

//-----------------------------------------------------------------------------
class AirspaceRule : public QObject
{
    Q_OBJECT
public:

    enum SelectionType {
      Pickone,              ///< One rule from the overall set needs to be picked.
      Required,             ///< Satisfying the RuleSet is required.
      Optional              ///< Satisfying the RuleSet is not required.
    };

    Q_ENUM(SelectionType)

    AirspaceRule(QObject* parent = NULL);

    Q_PROPERTY(QString          id              READ id             CONSTANT)
    Q_PROPERTY(QString          name            READ name           CONSTANT)
    Q_PROPERTY(QString          shortName       READ shortName      CONSTANT)
    Q_PROPERTY(QString          description     READ description    CONSTANT)
    Q_PROPERTY(bool             isDefault       READ isDefault      CONSTANT)
    Q_PROPERTY(SelectionType    selectionType   READ selectionType  CONSTANT)
    Q_PROPERTY(bool             selected        READ selected       WRITE setSelected   NOTIFY selectedChanged)

    virtual QString         id              () = 0;
    virtual QString         description     () = 0;
    virtual bool            isDefault       () = 0;
    virtual QString         name            () = 0;
    virtual QString         shortName       () = 0;
    virtual SelectionType   selectionType   () = 0;
    virtual bool            selected        () = 0;
    virtual void            setSelected     (bool sel) = 0;

signals:
    void    selectedChanged                 ();

};

//-----------------------------------------------------------------------------
class AirspaceRulesetsProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRulesetsProvider        (QObject* parent = NULL);
    ~AirspaceRulesetsProvider       () = default;

    Q_PROPERTY(bool                 valid           READ valid          NOTIFY rulesChanged)
    Q_PROPERTY(QString              selectedRules   READ selectedRules  NOTIFY selectedRulesChanged)
    Q_PROPERTY(QmlObjectListModel*  rules           READ rules          NOTIFY rulesChanged)

    virtual bool                valid       () = 0;             ///< Current ruleset is valid
    virtual QmlObjectListModel* rules       () = 0;             ///< List of AirspaceRule
    virtual QString            selectedRules() = 0;             ///< All selected rules concatenated into a string
    /**
     * Set region of interest that should be queried. When finished, the rulesChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void        setROI      (const QGeoCoordinate& center) = 0;

signals:
    void rulesChanged               ();
    void selectedRulesChanged       ();
};

