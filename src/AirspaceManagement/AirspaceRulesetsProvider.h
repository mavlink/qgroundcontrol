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
class AirspaceRuleFeature : public QObject
{
    Q_OBJECT
public:

    enum Type {
        Unknown,
        Boolean,
        Float,
        String,
    };

    enum Measurement {
        UnknownMeasurement,
        Speed,
        Weight,
        Distance,
    };

    enum Unit {
        UnknownUnit,
        Kilogram,
        Meters,
        MetersPerSecond,
    };

    Q_ENUM(Type)
    Q_ENUM(Measurement)
    Q_ENUM(Unit)

    AirspaceRuleFeature(QObject* parent = NULL);

    Q_PROPERTY(Type             type            READ type           CONSTANT)
    Q_PROPERTY(Measurement      measurement     READ measurement    CONSTANT)
    Q_PROPERTY(QString          name            READ name           CONSTANT)
    Q_PROPERTY(QString          description     READ description    CONSTANT)
    Q_PROPERTY(QVariant         value           READ description    CONSTANT)

    virtual Measurement     measurement     () = 0;
    virtual Type            type            () = 0;
    virtual QString         name            () = 0;
    virtual QString         description     () = 0;
    virtual QVariant        value           () = 0;
};

//-----------------------------------------------------------------------------
class AirspaceRule : public QObject
{
    Q_OBJECT
public:

    enum Status {
        Unknown,                ///< The status of the rule is unknown.
        Conflicting,            ///< The rule is conflicting.
        NotConflicting,         ///< The rule is not conflicting, all good to go.
        MissingInfo,            ///< The evaluation requires further information.
        Informational           ///< The rule is of informational nature.
    };

    Q_ENUM(Status)

    AirspaceRule(QObject* parent = NULL);

    Q_PROPERTY(Status           status          READ status         CONSTANT)
    Q_PROPERTY(QString          shortText       READ shortText      CONSTANT)
    Q_PROPERTY(QString          description     READ description    CONSTANT)

    virtual Status          status          () = 0;
    virtual QString         shortText       () = 0;
    virtual QString         description     () = 0;
};

//-----------------------------------------------------------------------------
class AirspaceRuleSet : public QObject
{
    Q_OBJECT
public:

    enum SelectionType {
      Pickone,              ///< One rule from the overall set needs to be picked.
      Required,             ///< Satisfying the RuleSet is required.
      Optional              ///< Satisfying the RuleSet is not required.
    };

    Q_ENUM(SelectionType)

    AirspaceRuleSet(QObject* parent = NULL);

    Q_PROPERTY(QString          id              READ id             CONSTANT)
    Q_PROPERTY(QString          name            READ name           CONSTANT)
    Q_PROPERTY(QString          shortName       READ shortName      CONSTANT)
    Q_PROPERTY(QString          description     READ description    CONSTANT)
    Q_PROPERTY(bool             isDefault       READ isDefault      CONSTANT)
    Q_PROPERTY(SelectionType    selectionType   READ selectionType  CONSTANT)
    Q_PROPERTY(bool             selected        READ selected       WRITE setSelected   NOTIFY selectedChanged)
    Q_PROPERTY(QmlObjectListModel* rules        READ rules          CONSTANT)

    virtual QString         id              () = 0;
    virtual QString         description     () = 0;
    virtual bool            isDefault       () = 0;
    virtual QString         name            () = 0;
    virtual QString         shortName       () = 0;
    virtual SelectionType   selectionType   () = 0;
    virtual bool            selected        () = 0;
    virtual void            setSelected     (bool sel) = 0;
    virtual QmlObjectListModel* rules       () = 0;             ///< List of AirspaceRule

signals:
    void    selectedChanged                 ();

};

//-----------------------------------------------------------------------------
class AirspaceRulesetsProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRulesetsProvider        (QObject* parent = NULL);
    ~AirspaceRulesetsProvider       () = default;

    Q_PROPERTY(bool                 valid               READ valid              NOTIFY ruleSetsChanged)
    Q_PROPERTY(QString              selectedRuleSets    READ selectedRuleSets   NOTIFY selectedRuleSetsChanged)
    Q_PROPERTY(QmlObjectListModel*  ruleSets            READ ruleSets           NOTIFY ruleSetsChanged)

    virtual bool                valid       () = 0;             ///< Current ruleset is valid
    virtual QmlObjectListModel* ruleSets    () = 0;             ///< List of AirspaceRuleSet
    virtual QString         selectedRuleSets() = 0;             ///< All selected rules concatenated into a string
    /**
     * Set region of interest that should be queried. When finished, the rulesChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void        setROI      (const QGeoCoordinate& center) = 0;

signals:
    void ruleSetsChanged            ();
    void selectedRuleSetsChanged    ();
};

