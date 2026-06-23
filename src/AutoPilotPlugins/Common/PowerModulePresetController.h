#pragma once

#include <QtCore/QVariantList>
#include <QtQmlIntegration/QtQmlIntegration>

#include "FactPanelController.h"

class PowerModulePresetController : public FactPanelController
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit PowerModulePresetController(QObject *parent = nullptr);
    ~PowerModulePresetController() override = default;

    Q_INVOKABLE QVariantList powerModulePresets() const;
};
