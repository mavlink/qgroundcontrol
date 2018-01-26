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
      pickone,              ///< One rule from the overall set needs to be picked.
      required,             ///< Satisfying the RuleSet is required.
      optional              ///< Satisfying the RuleSet is not required.
    };

    Q_ENUM(SelectionType)

    AirspaceRule(QObject* parent = NULL);

    Q_PROPERTY(QString          id              READ id             CONSTANT)
    Q_PROPERTY(QString          name            READ name           CONSTANT)
    Q_PROPERTY(QString          description     READ description    CONSTANT)
    Q_PROPERTY(bool             isDefault       READ isDefault      CONSTANT)
    Q_PROPERTY(SelectionType    selectionType   READ selectionType  CONSTANT)

    virtual QString         id              () = 0;
    virtual QString         description     () = 0;
    virtual bool            isDefault       () = 0;
    virtual QString         name            () = 0;
    virtual SelectionType   selectionType   () = 0;
};

//-----------------------------------------------------------------------------
class AirspaceRulesetsProvider : public QObject {
    Q_OBJECT
public:
    AirspaceRulesetsProvider        (QObject* parent = NULL);
    ~AirspaceRulesetsProvider       () = default;

    Q_PROPERTY(bool                 valid           READ valid          NOTIFY rulesChanged)
    Q_PROPERTY(QString              defaultRule     READ defaultRule    NOTIFY rulesChanged)
    Q_PROPERTY(int                  defaultIndex    READ defaultIndex   NOTIFY rulesChanged)
    Q_PROPERTY(int                  currentIndex    READ currentIndex   WRITE  setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QmlObjectListModel*  rules           READ rules          NOTIFY rulesChanged)

    virtual bool                valid       () = 0;             ///< Current ruleset is valid
    virtual QmlObjectListModel* rules       () = 0;             ///< List of AirspaceRule
    virtual QString             defaultRule () = 0;             ///< AirspaceRule::name
    virtual int                 defaultIndex() = 0;             ///< Index into rules (QmlObjectListModel)
    virtual int                 currentIndex() = 0;             ///< User selected index into rules (QmlObjectListModel)
    virtual void             setCurrentIndex(int index) = 0;    ///< Select a rule
    /**
     * Set region of interest that should be queried. When finished, the rulesChanged() signal will be emmited.
     * @param center Center coordinate for ROI
     */
    virtual void        setROI      (const QGeoCoordinate& center) = 0;

signals:
    void rulesChanged               ();
    void currentIndexChanged        ();
};

