#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class DigiviewManager;

Q_MOC_INCLUDE("DigiviewManager.h")

class SVBackend final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(SVBackend)
    QML_SINGLETON

    Q_PROPERTY(DigiviewManager* digiview READ digiview CONSTANT)

public:
    explicit SVBackend(QObject* parent = nullptr);

    DigiviewManager* digiview() const;
};
