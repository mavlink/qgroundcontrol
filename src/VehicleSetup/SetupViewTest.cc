/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "SetupViewTest.h"
#include "MockLink.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"

void SetupViewTest::_clickThrough_test(void)
{    
    _createMainWindow();
    _connectMockLink();

    AutoPilotPlugin* autopilot = qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()->autopilotPlugin();
    Q_ASSERT(autopilot);

    // Switch to the Setup view
    qgcApp()->showSetupView();
    QTest::qWait(1000);
    
    // Click through fixed buttons
    qDebug() << "Showing firmware";
    qgcApp()->_showSetupFirmware();
    QTest::qWait(1000);
    qDebug() << "Showing parameters";
    qgcApp()->_showSetupParameters();
    QTest::qWait(1000);
    qDebug() << "Showing summary";
    qgcApp()->_showSetupSummary();
    QTest::qWait(1000);
    
    const QVariantList& components = autopilot->vehicleComponents();
    foreach(QVariant varComponent, components) {
        VehicleComponent* component = qobject_cast<VehicleComponent*>(qvariant_cast<QObject *>(varComponent));
        qDebug() << "Showing" << component->name();
        qgcApp()->_showSetupVehicleComponent(component);
        QTest::qWait(1000);
    }

    _disconnectMockLink();
    _closeMainWindow();
}
