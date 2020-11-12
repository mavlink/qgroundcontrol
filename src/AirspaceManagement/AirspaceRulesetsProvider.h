/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
#include "QGCGeoBoundingCube.h"

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

    AirspaceRuleFeature(QObject* parent = nullptr);

    Q_PROPERTY(quint32          id              READ id             CONSTANT)
    Q_PROPERTY(Type             type            READ type           CONSTANT)
    Q_PROPERTY(Unit             unit            READ unit           CONSTANT)
    Q_PROPERTY(Measurement      measurement     READ measurement    CONSTANT)
    Q_PROPERTY(QString          name            READ name           CONSTANT)
    Q_PROPERTY(QString          description     READ description    CONSTANT)
    Q_PROPERTY(QVariant         value           READ value          WRITE setValue  NOTIFY valueChanged)

    virtual quint32         id              () = 0;
    virtual Type            type            () = 0;
    virtual Unit            unit            () = 0;
    virtual Measurement     measurement     () = 0;
    virtual QString         name            () = 0;
    virtual QString         description     () = 0;
    virtual QVariant        value           () = 0;
    virtual void            setValue        (const QVariant val) = 0;

signals:
    void valueChanged   ();
};

//-----------------------------------------------------------------------------
class AirspaceRule : public QObject
{
    Q_OBJECT
public:

    enum Status {
        Conflicting,            ///< The rule is conflicting.
        MissingInfo,            ///< The evaluation requires further information.
        NotConflicting,         ///< The rule is not conflicting, all good to go.
        Informational,          ///< The rule is of informational nature.
        Unknown,                ///< The status of the rule is unknown.
    };

    Q_ENUM(Status)

    AirspaceRule(QObject* parent = nullptr);

    Q_PROPERTY(Status               status          READ status         CONSTANT)
    Q_PROPERTY(QString              shortText       READ shortText      CONSTANT)
    Q_PROPERTY(QString              description     READ description    CONSTANT)
    Q_PROPERTY(QmlObjectListModel*  features        READ features       CONSTANT)

    virtual Status              status          () = 0;
    virtual QString             shortText       () = 0;
    virtual QString             description     () = 0;
    virtual QmlObjectListModel* features        () = 0;     ///< List of AirspaceRuleFeature
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

    AirspaceRuleSet(QObject* parent = nullptr);

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
    AirspaceRulesetsProvider        (QObject* parent = nullptr);
    ~AirspaceRulesetsProvider       () = default;

    Q_PROPERTY(bool                 valid               READ valid              NOTIFY ruleSetsChanged)
    Q_PROPERTY(QString              selectedRuleSets    READ selectedRuleSets   NOTIFY selectedRuleSetsChanged)
    Q_PROPERTY(QmlObjectListModel*  ruleSets            READ ruleSets           NOTIFY ruleSetsChanged)

    Q_INVOKABLE virtual void    clearAllFeatures() {;}          ///< Clear all saved (persistent) feature values

    virtual bool                valid       () = 0;             ///< Current ruleset is valid
    virtual QmlObjectListModel* ruleSets    () = 0;             ///< List of AirspaceRuleSet
    virtual QString         selectedRuleSets() = 0;             ///< All selected rules concatenated into a string
    /**
     * Set region of interest that should be queried. When finished, the rulesChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void        setROI      (const QGCGeoBoundingCube& roi, bool reset = false) = 0;

signals:
    void ruleSetsChanged            ();
    void selectedRuleSetsChanged    ();
};

