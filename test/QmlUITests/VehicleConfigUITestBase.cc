#include "VehicleConfigUITestBase.h"

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickWindow>
#include <QtTest/QTest>

#include "AutoPilotPlugin.h"
#include "Fact.h"
#include "ParameterManager.h"
#include "Vehicle.h"
#include "VehicleComponent.h"

void VehicleConfigUITestBase::navigateToConfigureView()
{
    QVERIFY2(clickToolSelectDropdownButton(QStringLiteral("toolbar_viewConfigure")),
             "Failed to navigate to Configure view");
    QTest::qWait(_viewDelay);

    QVERIFY2(findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_root"), 3000),
             "vehicleConfig_root not found after navigating to Configure");
}

QQuickItem *VehicleConfigUITestBase::clickSidebarButton(const QString &objectName)
{
    QQuickItem *btn = findVisibleItem(_rootItem, objectName, 3000);
    if (!btn) {
        QTest::qFail(qPrintable(QStringLiteral("Sidebar button not found: %1").arg(objectName)), __FILE__, __LINE__);
        return nullptr;
    }

    scrollIntoView(btn, QStringLiteral("vehicleConfig_sidebarFlickable"));
    const QPointF center = btn->mapToScene(QPointF(btn->width() / 2, btn->height() / 2));
    QTest::mouseClick(_window, Qt::LeftButton, Qt::NoModifier, center.toPoint());
    QTest::qWait(_pageDelay);

    return btn;
}

void VehicleConfigUITestBase::resetParamsToFirmwareDefaults(Vehicle *vehicle, const QString &sentinelParamName)
{
    ParameterManager *mgr = vehicle->parameterManager();

    // The sentinel parameter is non-default in the MockLink params file but
    // defaults to 0 in the parameter metadata, so it signals the refresh completing
    Fact *sentinelFact = mgr->getParameter(ParameterManager::defaultComponentId, sentinelParamName);
    QVERIFY2(sentinelFact, qPrintable(QStringLiteral("%1 fact not found").arg(sentinelParamName)));
    QVERIFY2(sentinelFact->rawValue().toInt() != 0,
             qPrintable(QStringLiteral("%1 already at default before reset").arg(sentinelParamName)));

    // Sends MAV_CMD_PREFLIGHT_STORAGE param1=2 which MockLink handles by resetting
    // its parameters to the metadata defaults
    mgr->resetAllParametersToDefaults();
    mgr->refreshAllParameters();

    QVERIFY2(QTest::qWaitFor([&] { return sentinelFact->rawValue().toInt() == 0; }, 30000),
             "Parameters never refreshed to firmware defaults");
}

void VehicleConfigUITestBase::resetAPMParamsToUncalibrated(Vehicle *vehicle)
{
    ParameterManager *mgr = vehicle->parameterManager();

    QVERIFY2(mgr->parameterExists(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X")),
             "COMPASS_OFS_X parameter not found");
    Fact *compassOfs = mgr->getParameter(ParameterManager::defaultComponentId, QStringLiteral("COMPASS_OFS_X"));
    QVERIFY2(compassOfs, "COMPASS_OFS_X fact not found");

    // Sends MAV_CMD_PREFLIGHT_STORAGE param1=2 which MockLink's APM branch zeroes
    // the compass and accel offset parameters.
    mgr->resetAllParametersToDefaults();
    mgr->refreshAllParameters();

    QVERIFY2(QTest::qWaitFor([&] { return qFuzzyIsNull(compassOfs->rawValue().toFloat()); }, 30000),
             "COMPASS_OFS_X never refreshed to 0 after APM param reset");
}

void VehicleConfigUITestBase::clickThroughAllComponents(Vehicle *vehicle, const QString &vehicleName)
{
    const QString prefix = vehicleName.isEmpty() ? QString() : (vehicleName + QStringLiteral(": "));

    const QVariantList components = vehicle->autopilotPlugin()->vehicleComponents();
    QVERIFY2(!components.isEmpty(),
             qPrintable(QStringLiteral("%1No vehicle components found").arg(prefix)));

    for (const QVariant &compVariant : components) {
        auto *comp = compVariant.value<VehicleComponent *>();
        if (!comp) {
            continue;
        }

        // Match the objectName set in VehicleConfigView.qml:
        // "vehicleConfig_comp_" + compName.replace(/ /g, "")
        const QString cleanName  = QString(comp->name()).remove(QLatin1Char(' '));
        const QString buttonName = QStringLiteral("vehicleConfig_comp_") + cleanName;

        QQuickItem *btn = findVisibleItem(_rootItem, buttonName, 2000);
        if (!btn) {
            // Component may be hidden (e.g. optional peripheral not present) – skip silently
            continue;
        }

        clickSidebarButton(buttonName);
        if (QTest::currentTestFailed()) return;

        QQuickItem *loader = findVisibleItem(_rootItem, QStringLiteral("vehicleConfig_panelLoader"), 2000);
        QVERIFY2(loader,
                 qPrintable(QStringLiteral("%1vehicleConfig_panelLoader not found after clicking %2")
                                .arg(prefix, comp->name())));
        QVERIFY2(loader->property("item").value<QQuickItem *>() != nullptr,
                 qPrintable(QStringLiteral("%1Panel loader has no item after clicking %2")
                                .arg(prefix, comp->name())));
    }
}
