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

#include "APMAirframeComponentController.h"
#include "APMAirframeComponentAirframes.h"
#include "QGCMAVLink.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "ParameterManager.h"

#include <QVariant>
#include <QQmlProperty>
#include <QStandardPaths>
#include <QDir>
#include <QJsonParseError>
#include <QJsonObject>

bool APMAirframeComponentController::_typesRegistered = false;

const char* APMAirframeComponentController::_oldFrameParam = "FRAME";
const char* APMAirframeComponentController::_newFrameParam = "FRAME_CLASS";

APMAirframeComponentController::APMAirframeComponentController(void) :
    _airframeTypesModel(new QmlObjectListModel(this))
{
    if (!_typesRegistered) {
        _typesRegistered = true;
        qmlRegisterUncreatableType<APMAirframeType>("QGroundControl.Controllers", 1, 0, "APMAirframeType", QStringLiteral("Can only reference APMAirframeType"));
    }
    _fillAirFrames();

    Fact* frame;
    if (parameterExists(FactSystem::defaultComponentId, _oldFrameParam)) {
        frame = getParameterFact(FactSystem::defaultComponentId, _oldFrameParam);
    } else {
        frame = getParameterFact(FactSystem::defaultComponentId, _newFrameParam);
    }
    connect(frame, &Fact::rawValueChanged, this, &APMAirframeComponentController::_factFrameChanged);
    _factFrameChanged(frame->rawValue());
}

APMAirframeComponentController::~APMAirframeComponentController()
{

}

void APMAirframeComponentController::_factFrameChanged(QVariant value)
{
    FrameId v = (FrameId) value.toInt();

    for(int i = 0, size = _airframeTypesModel->count(); i < size; i++ ) {
        APMAirframeType *airframeType = qobject_cast<APMAirframeType*>(_airframeTypesModel->get(i));
        Q_ASSERT(airframeType);
        if (airframeType->type() == v) {
            _currentAirframeType = airframeType;
            break;
        }
    }
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

void APMAirframeComponentController::_loadParametersFromDownloadFile(const QString& downloadedParamFile)
{
    QFile parametersFile(downloadedParamFile);
    if (!parametersFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open downloaded parameter file" << downloadedParamFile << parametersFile.errorString();
        qgcApp()->restoreOverrideCursor();
        return;
    }

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
    qgcApp()->restoreOverrideCursor();
    _vehicle->parameterManager()->refreshAllParameters();
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

int APMAirframeType::type() const
{
    return _type;
}

APMAirframeType *APMAirframeComponentController::currentAirframeType() const
{
    return _currentAirframeType;
}

QString APMAirframeComponentController::currentAirframeTypeName() const
{
    return _vehicle->vehicleTypeName();
}

void APMAirframeComponentController::setCurrentAirframeType(APMAirframeType *t)
{
    Fact *param = getParameterFact(-1, QStringLiteral("FRAME"));
    Q_ASSERT(param);
    param->setRawValue(t->type());
}

void APMAirframeComponentController::loadParameters(const QString& paramFile)
{
    qgcApp()->setOverrideCursor(Qt::WaitCursor);

    QString paramFileUrl = QStringLiteral("https://api.github.com/repos/ArduPilot/ardupilot/contents/Tools/Frame_params/%1?ref=master");

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &APMAirframeComponentController::_githubJsonDownloadFinished);
    connect(downloader, &QGCFileDownload::error, this, &APMAirframeComponentController::_githubJsonDownloadError);
    downloader->download(paramFileUrl.arg(paramFile));
}

void APMAirframeComponentController::_githubJsonDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    QFile jsonFile(localFile);
    if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Unable to open github json file" << localFile << jsonFile.errorString();
        qgcApp()->restoreOverrideCursor();
        return;
    }
    QByteArray bytes = jsonFile.readAll();
    jsonFile.close();

    QJsonParseError jsonParseError;
    QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
    if (jsonParseError.error != QJsonParseError::NoError) {
        qWarning() <<  "Unable to open json document" << localFile << jsonParseError.errorString();
        qgcApp()->restoreOverrideCursor();
        return;
    }
    QJsonObject json = doc.object();

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &APMAirframeComponentController::_paramFileDownloadFinished);
    connect(downloader, &QGCFileDownload::error, this, &APMAirframeComponentController::_paramFileDownloadError);
    downloader->download(json["download_url"].toString());
}

void APMAirframeComponentController::_githubJsonDownloadError(QString errorMsg)
{
    qgcApp()->showMessage(tr("Param file github json download failed: %1").arg(errorMsg));
    qgcApp()->restoreOverrideCursor();
}

void APMAirframeComponentController::_paramFileDownloadFinished(QString remoteFile, QString localFile)
{
    Q_UNUSED(remoteFile);

    _loadParametersFromDownloadFile(localFile);
}

void APMAirframeComponentController::_paramFileDownloadError(QString errorMsg)
{
    qgcApp()->showMessage(tr("Param file download failed: %1").arg(errorMsg));
    qgcApp()->restoreOverrideCursor();
}
