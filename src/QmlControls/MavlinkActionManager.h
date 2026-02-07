#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>


class Fact;
class QmlObjectListModel;

/// Loads the specified action file and provides access to the actions it contains.
/// Action files are loaded from the default MavlinkActions directory.
/// The actions file name is filename only, no path.
class MavlinkActionManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_MOC_INCLUDE("Fact.h")
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(Fact* actionFileNameFact READ actionFileNameFact WRITE setActionFileNameFact NOTIFY actionFileNameFactChanged)
    Q_PROPERTY(QmlObjectListModel* actions READ actions CONSTANT)

public:
    explicit MavlinkActionManager(QObject *parent = nullptr);
    explicit MavlinkActionManager(Fact *actionFileNameFact, QObject *parent = nullptr);
    ~MavlinkActionManager();

    Fact *actionFileNameFact() { return _actionFileNameFact; }
    void setActionFileNameFact(Fact *actionFileNameFact);
    QmlObjectListModel *actions() { return _actions; }

signals:
    void actionFileNameFactChanged();

private slots:
    void _loadActionsFile();

private:
    Fact *_actionFileNameFact = nullptr;
    QmlObjectListModel *_actions = nullptr;
};
