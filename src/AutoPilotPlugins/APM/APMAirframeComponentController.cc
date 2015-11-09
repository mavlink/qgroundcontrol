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
    _showCustomConfigPanel(false),
    _airframeTypesModel(new QmlObjectListModel(this))
{
    if (!_typesRegistered) {
        _typesRegistered = true;
        qmlRegisterUncreatableType<APMAirframeType>("QGroundControl.Controllers", 1, 0, "APMAiframeType", "Can only reference APMAirframeType");
        qmlRegisterUncreatableType<APMAirframe>("QGroundControl.Controllers", 1, 0, "APMAiframe", "Can only reference APMAirframe");
    }
    _fillAirFrames();

    Fact *frame = getParameterFact(FactSystem::defaultComponentId, "FRAME");
    connect(frame, &Fact::vehicleUpdated, this, &APMAirframeComponentController::_factFrameChanged);
    _factFrameChanged(frame->rawValue());
}

APMAirframeComponentController::~APMAirframeComponentController()
{

}

void APMAirframeComponentController::_factFrameChanged(QVariant value)
{
    FrameId v(value.value<FrameId>());
    _currentAirframeType = v;
    emit currentAirframeTypeChanged(_currentAirframeType);
}

void APMAirframeComponentController::_fillAirFrames()
{
    for (int tindex = 0; tindex < APMAirframeComponentAirframes::get().count(); tindex++) {
        const APMAirframeComponentAirframes::AirframeType_t* pType = APMAirframeComponentAirframes::get().values().at(tindex);

        APMAirframeType* airframeType = new APMAirframeType(pType->name, pType->imageResource, pType->type, this);
        Q_CHECK_PTR(airframeType);

        for (int index = 0; index < pType->rgAirframeInfo.count(); index++) {
            const APMAirframe* pInfo = pType->rgAirframeInfo.at(index);
            Q_CHECK_PTR(pInfo);

            airframeType->addAirframe(pInfo->name(), pInfo->params(), pInfo->type());
        }
        _airframeTypesModel->append(airframeType);
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
    qDebug() << "Starting vehicle update";
    APMRemoteParamsDownloader *paramDownloader = new APMRemoteParamsDownloader(_currentFileParams);
    connect(paramDownloader, &APMRemoteParamsDownloader::finished, this, &APMAirframeComponentController::_finishVehicleSetup);
}

void APMAirframeComponentController::_finishVehicleSetup() {
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
        if (parameterExists(-1, aux.at(0))) {
            Fact *param = getParameterFact(-1, aux.at(0));
            param->setRawValue(QVariant::fromValue(aux.at(1)));
        }
    }
    qgcApp()->setOverrideCursor(Qt::ArrowCursor);
    qDebug() << "Finished Vehicle setup";
}

APMAirframeType::APMAirframeType(const QString& name, const QString& imageResource, int type, QObject* parent) :
    QObject(parent),
    _name(name),
    _imageResource(imageResource),
    _type(type),
    _dirty(false)
{
}

APMAirframeType::~APMAirframeType()
{
}

void APMAirframeType::addAirframe(const QString& name, const QString& file, int type)
{
    APMAirframe* airframe = new APMAirframe(name, file, type);
    Q_CHECK_PTR(airframe);
    
    _airframes.append(QVariant::fromValue(airframe));
}

APMAirframe::APMAirframe(const QString& name, const QString& paramsFile, int type, QObject* parent) :
    QObject(parent),
    _name(name),
    _paramsFile(paramsFile),
    _type(type)
{
}

QString APMAirframe::name() const
{
    return _name;
}

QString APMAirframe::params() const
{
    return _paramsFile;
}

int APMAirframe::type() const
{
    return _type;
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
