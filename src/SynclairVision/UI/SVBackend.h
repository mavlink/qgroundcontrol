#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class DigiviewManager;
class QJSEngine;
class QQmlEngine;

Q_MOC_INCLUDE("DigiviewManager.h")

class SVBackend final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SVBackend)
    QML_SINGLETON

    Q_PROPERTY(DigiviewManager* digiview READ digiview CONSTANT)

public:
    explicit SVBackend(DigiviewManager* digiview, QObject* parent = nullptr);

    static SVBackend* create(QQmlEngine* qmlEngine, QJSEngine* jsEngine);

    DigiviewManager* digiview() const { return _digiview; }

private:
    DigiviewManager* const _digiview;
};
