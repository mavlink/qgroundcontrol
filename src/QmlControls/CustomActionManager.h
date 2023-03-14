/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QObject>
#include <QmlObjectListModel.h>


class CustomActionManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QmlObjectListModel*  actions     READ  actions     NOTIFY  actionsChanged)
    Q_PROPERTY(bool                 hasActions  READ  hasActions  NOTIFY  actionsChanged)

public:
    CustomActionManager(void);

    QmlObjectListModel* actions(void) { return &_actions; }
    bool hasActions(void) { return  _actions.count() > 0; }

signals:
    void actionsChanged();

private slots:
    void _loadFromJson(QVariant path);

private:
    QmlObjectListModel  _actions;
    bool _hasActions;

};
