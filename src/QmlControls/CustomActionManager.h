/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "Fact.h"
#include "QmlObjectListModel.h"

#include <QtCore/QObject>

/// Loads the specified action file and provides access to the actions it contains.
/// Action files are loaded from the default CustomActions directory.
/// The actions file name is filename only, no path.
class CustomActionManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Fact*                actionFileNameFact  READ  actionFileNameFact WRITE setActionFileNameFact    NOTIFY actionFileNameFactChanged)
    Q_PROPERTY(QmlObjectListModel*  actions             READ  actions                                           CONSTANT)

public:
    CustomActionManager(QObject* parent = nullptr);
    CustomActionManager(Fact* actionFileNameFact, QObject* parent = nullptr);

    Fact*               actionFileNameFact      (void) { return _actionFileNameFact; }
    void                setActionFileNameFact   (Fact* actionFileNameFact);
    QmlObjectListModel* actions                 (void) { return &_actions; }

signals:
    void actionFileNameFactChanged();

private slots:
    void _loadActionsFile(void);

private:
    Fact*               _actionFileNameFact;
    QmlObjectListModel  _actions;
};
