/*=====================================================================
 
 QGroundControl Open Source Ground Control Station
 
 (c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 
 This file is part of the QGROUNDCONTROL project
 
 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.
 
 ======================================================================*/

/// @file
///     @author Don Gagne <don@thegagnes.com>

#include "APMAirframeComponentController.h"
#include "APMAirframeComponentAirframes.h"
#include "APMRemoteParamsDownloader.h"
#include "QGCMAVLink.h"
#include "MultiVehicleManager.h"
#include "AutoPilotPluginManager.h"
#include "QGCApplication.h"
#include "QGCMessageBox.h"

#include <QVariant>
#include <QQmlProperty>

bool APMAirframeComponentController::_typesRegistered = false;

APMAirframeComponentController::APMAirframeComponentController(void) :
    _currentVehicleIndex(0),
    _autostartId(0),
    _showCustomConfigPanel(false),
    _airframeTypesModel(new QmlObjectListModel(this))
{
    if (!_typesRegistered) {
        _typesRegistered = true;
        qmlRegisterUncreatableType<APMAirframeType>("QGroundControl.Controllers", 1, 0, "APMAiframeType", "Can only reference APMAirframeType");
        qmlRegisterUncreatableType<APMAirframe>("QGroundControl.Controllers", 1, 0, "APMAiframe", "Can only reference APMAirframe");
    }

    APMRemoteParamsDownloader *paramDownloader = new APMRemoteParamsDownloader();
    connect(paramDownloader, SIGNAL(finished()), this, SLOT(_fillAirFrames()));
}

APMAirframeComponentController::~APMAirframeComponentController()
{

}

void APMAirframeComponentController::_fillAirFrames()
{
    // Load up member variables
    bool autostartFound = false;
    _autostartId = 0; //getParameterFact(FactSystem::defaultComponentId, "SYS_AUTOSTART")->value().toInt();

    QList<APMAirframeType*> airframeTypes;
    for (int tindex = 0; tindex < APMAirframeComponentAirframes::get().count(); tindex++) {
        const APMAirframeComponentAirframes::AirframeType_t* pType = APMAirframeComponentAirframes::get().values().at(tindex);

        APMAirframeType* airframeType = new APMAirframeType(pType->name, pType->imageResource, this);
        Q_CHECK_PTR(airframeType);

        for (int index = 0; index < pType->rgAirframeInfo.count(); index++) {
            const APMAirframeComponentAirframes::AirframeInfo_t* pInfo = pType->rgAirframeInfo.at(index);
            Q_CHECK_PTR(pInfo);

            if (_autostartId == pInfo->autostartId) {
                Q_ASSERT(!autostartFound);
                autostartFound = true;
                _currentAirframeType = pType->name;
                _currentVehicleName = pInfo->name;
                _currentVehicleIndex = index;
            }
            airframeType->addAirframe(pInfo->name, pInfo->file, pInfo->autostartId);
        }
        _airframeTypesModel->append(airframeType);
    }

    if (_autostartId != 0 && !autostartFound) {
        _showCustomConfigPanel = true;
        emit showCustomConfigPanelChanged(true);
    }
    emit loadAirframesCompleted();
}

void APMAirframeComponentController::changeAutostart(void)
{
    if (qgcApp()->toolbox()->multiVehicleManager()->vehicles()->count() > 1) {
        QGCMessageBox::warning("APMAirframe Config", "You cannot change Airframe configuration while connected to multiple vehicles.");
        return;
    }

    qgcApp()->setOverrideCursor(Qt::WaitCursor);
    QDir dataLocation = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation).at(0)
           + QDir::separator() + qApp->applicationName();

    QFile parametersFile(dataLocation.absoluteFilePath(_currentFileParams));
    parametersFile.open(QIODevice::ReadOnly);

    QTextStream reader(&parametersFile);

    while (!reader.atEnd()) {
        QString line = reader.readLine().trimmed();
        if (line.isEmpty() || line.at(0) == QChar('#')) {
            continue;
        }

        QStringList aux = line.split(',');
        Fact *param = getParameterFact(-1, aux.at(0));
        if (!param) {
            qDebug() << "Parameter not found:" << aux.at(0);
            continue;
        }
        param->setRawValue(QVariant::fromValue(aux.at(1)));
        qDebug() << "Setting the parameters" << aux;
    }
   qgcApp()->setOverrideCursor(Qt::ArrowCursor);
}

APMAirframeType::APMAirframeType(const QString& name, const QString& imageResource, QObject* parent) :
    QObject(parent),
    _name(name),
    _imageResource(imageResource),
    _dirty(false)
{
}

APMAirframeType::~APMAirframeType()
{
}

void APMAirframeType::addAirframe(const QString& name, const QString& file, int autostartId)
{
    APMAirframe* airframe = new APMAirframe(name, file, autostartId);
    Q_CHECK_PTR(airframe);
    
    _airframes.append(QVariant::fromValue(airframe));
}

APMAirframe::APMAirframe(const QString& name, const QString& paramsFile, int autostartId, QObject* parent) :
    QObject(parent),
    _name(name),
    _paramsFile(paramsFile),
    _autostartId(autostartId)
{
}

APMAirframe::~APMAirframe()
{
}

QString APMAirframeType::imageResource() const
{
    return _imageResource;
}

QString APMAirframeType::name() const
{
    return _name;
}
