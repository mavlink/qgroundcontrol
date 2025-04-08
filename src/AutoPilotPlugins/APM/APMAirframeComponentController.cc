/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMAirframeComponentController.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"
#include "MultiVehicleManager.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "QGCLoggingCategory.h"
#include "Vehicle.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QVariant>
#include <QtGui/QCursor>
#include <QtGui/QGuiApplication>

QGC_LOGGING_CATEGORY(APMAirframeComponentControllerLog, "qgc.autopilotplugins.apm.apmairframecomponentcontroller")

/*===========================================================================*/

APMAirframeComponentController::APMAirframeComponentController(QObject *parent)
    : FactPanelController(parent)
    , _frameClassFact(getParameterFact(ParameterManager::defaultComponentId, QStringLiteral("FRAME_CLASS"), false /* reportMissing */))
    , _frameTypeFact(getParameterFact(ParameterManager::defaultComponentId, QStringLiteral("FRAME_TYPE"), false /* reportMissing */))
    , _frameClassModel(new QmlObjectListModel(this))
{
    // qCDebug(APMAirframeComponentControllerLog) << Q_FUNC_INFO << this;

    _fillFrameClasses();
}

APMAirframeComponentController::~APMAirframeComponentController()
{
    // qCDebug(APMAirframeComponentControllerLog) << Q_FUNC_INFO << this;
}

void APMAirframeComponentController::_fillFrameClasses()
{
    FirmwarePlugin *const fwPlugin = _vehicle->firmwarePlugin();

    if (qobject_cast<ArduCopterFirmwarePlugin*>(fwPlugin)) {
        static const QList<int> frameTypeNotSupported = {
            FRAME_CLASS_HELI,
            FRAME_CLASS_SINGLECOPTER,
            FRAME_CLASS_COAXCOPTER,
            FRAME_CLASS_BICOPTER,
            FRAME_CLASS_HELI_DUAL,
            FRAME_CLASS_HELIQUAD
        };

        for (qsizetype i = 1; i < _frameClassFact->enumStrings().count(); i++) {
            const QString frameClassName = _frameClassFact->enumStrings()[i];
            const int frameClass = _frameClassFact->enumValues()[i].toInt();

            if (frameClass == FRAME_CLASS_HELI) {
                // Heli requires it's own firmware variant. You can't switch to Heli from a Copter variant firmware.
                continue;
            }

            _frameClassModel->append(new APMFrameClass(frameClassName, true /* copter */, frameClass, _frameTypeFact, _frameClassModel));
        }
    } else if (qobject_cast<ArduRoverFirmwarePlugin*>(fwPlugin)) {
        for (qsizetype i = 1; i < _frameClassFact->enumStrings().count(); i++) {
            const QString frameClassName = _frameClassFact->enumStrings()[i];
            const int frameClass = _frameClassFact->enumValues()[i].toInt();
            _frameClassModel->append(new APMFrameClass(frameClassName, false /* copter */, frameClass, _frameTypeFact, _frameClassModel));
        }
    }
}

void APMAirframeComponentController::_loadParametersFromDownloadFile(const QString &downloadedParamFile)
{
    QFile parametersFile(downloadedParamFile);
    if (!parametersFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(APMAirframeComponentControllerLog) << "Unable to open downloaded parameter file" << downloadedParamFile << parametersFile.errorString();
        QGuiApplication::restoreOverrideCursor();
        return;
    }

    QTextStream reader(&parametersFile);
    while (!reader.atEnd()) {
        const QString line = reader.readLine().trimmed();
        if (line.isEmpty() || (line.at(0) == QChar('#'))) {
            continue;
        }

        const QStringList aux = line.split(',');
        if (parameterExists(-1, aux.at(0))) {
            Fact *const param = getParameterFact(-1, aux.at(0));
            param->setRawValue(QVariant::fromValue(aux.at(1)));
        }
    }
    QGuiApplication::restoreOverrideCursor();
    _vehicle->parameterManager()->refreshAllParameters();
}

void APMAirframeComponentController::loadParameters(const QString &paramFile)
{
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    QGCFileDownload *const downloader = new QGCFileDownload(this);
    (void) connect(downloader, &QGCFileDownload::downloadComplete, this, &APMAirframeComponentController::_githubJsonDownloadComplete);
    const QString paramFileUrl = QStringLiteral("https://api.github.com/repos/ArduPilot/ardupilot/contents/Tools/Frame_params/%1?ref=master");
    if (!downloader->download(paramFileUrl.arg(paramFile))) {
        downloader->deleteLater();
    }
}

void APMAirframeComponentController::_githubJsonDownloadComplete(const QString& /*remoteFile*/, const QString &localFile, const QString &errorMsg)
{
    if (errorMsg.isEmpty()) {
        QFile jsonFile(localFile);
        if (!jsonFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qCWarning(APMAirframeComponentControllerLog) << "Unable to open github json file" << localFile << jsonFile.errorString();
            QGuiApplication::restoreOverrideCursor();
            return;
        }
        const QByteArray bytes = jsonFile.readAll();
        jsonFile.close();

        QJsonParseError jsonParseError;
        const QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonParseError);
        if (jsonParseError.error != QJsonParseError::NoError) {
            qCWarning(APMAirframeComponentControllerLog) <<  "Unable to open json document" << localFile << jsonParseError.errorString();
            QGuiApplication::restoreOverrideCursor();
            return;
        }

        QGCFileDownload *const downloader = new QGCFileDownload(this);
        (void) connect(downloader, &QGCFileDownload::downloadComplete, this, &APMAirframeComponentController::_paramFileDownloadComplete);
        const QJsonObject json = doc.object();
        if (!downloader->download(json[QLatin1String("download_url")].toString())) {
            downloader->deleteLater();
        }
    } else {
        qgcApp()->showAppMessage(tr("Param file github json download failed: %1").arg(errorMsg));
        QGuiApplication::restoreOverrideCursor();
    }
}

void APMAirframeComponentController::_paramFileDownloadComplete(const QString& /*remoteFile*/, const QString &localFile, const QString &errorMsg)
{
    if (errorMsg.isEmpty()) {
        _loadParametersFromDownloadFile(localFile);
    } else {
        qgcApp()->showAppMessage(tr("Param file download failed: %1").arg(errorMsg));
        QGuiApplication::restoreOverrideCursor();
    }
}

/*===========================================================================*/

APMFrameClass::APMFrameClass(const QString &name, bool copter, int frameClass, Fact *frameTypeFact, QObject *parent)
    : QObject(parent)
    , _name(name)
    , _copter(copter)
    , _frameClass(frameClass)
    , _frameTypeFact(frameTypeFact)
{
    if (frameTypeFact) {
        (void) connect(frameTypeFact, &Fact::rawValueChanged, this, &APMFrameClass::imageResourceChanged);
        (void) connect(frameTypeFact, &Fact::rawValueChanged, this, &APMFrameClass::frameTypeChanged);
    }

    if (copter) {
        QList<int> rgSupportedFrameTypes;

        for (const FrameToImageInfo &pFrameToImageInfo : _rgFrameToImageCopter) {
            if (pFrameToImageInfo.frameClass == frameClass) {
                if (_defaultFrameType == -1) {
                    // Default frame type/icon is the first item found to match frameClass
                    _defaultFrameType = pFrameToImageInfo.frameType;
                    _imageResourceDefault = QStringLiteral("/qmlimages/Airframe/%1").arg(pFrameToImageInfo.imageResource);
                }

                if (pFrameToImageInfo.frameType != -1) {
                    // The list includes the supported frame types for the class
                    rgSupportedFrameTypes.append(pFrameToImageInfo.frameType);
                }
            }
        }

        if (_imageResourceDefault.isEmpty()) {
            _imageResourceDefault = QStringLiteral("/qmlimages/Airframe/AirframeUnknown.svg");
        }

        // Filter the enums
        for (const int frameType: rgSupportedFrameTypes) {
            const int index = frameTypeFact->enumValues().indexOf(frameType);
            if (index != -1) {
                _frameTypeEnumValues.append(frameType);
                _frameTypeEnumStrings.append(frameTypeFact->enumStrings()[index]);
            }
        }
    } else {
        _imageResourceDefault = imageResource();
    }

    // If the frameClass is not in the list then frame type is not supported
    _frameTypeSupported = _defaultFrameType != -1;
}

APMFrameClass::~APMFrameClass()
{

}

int APMFrameClass::frameType() const
{
    return _frameTypeFact->rawValue().toInt();
}

QString APMFrameClass::imageResource() const
{
    int frameType = _frameTypeFact ? _frameTypeFact->rawValue().toInt() : -1;

    QString imageResource;
    if (_copter) {
        imageResource = _findImageResourceCopter(_frameClass, frameType);
    } else {
        imageResource = _findImageResourceRover(_frameClass, frameType);
    }

    return QStringLiteral("/qmlimages/Airframe/%1").arg(imageResource);
}


QString APMFrameClass::_findImageResourceCopter(int frameClass, int &frameType)
{
    for (const FrameToImageInfo &pFrameToImageInfo : _rgFrameToImageCopter) {
        if (((pFrameToImageInfo.frameClass == frameClass) && (frameType == -1)) ||
            ((pFrameToImageInfo.frameClass == frameClass) && (pFrameToImageInfo.frameType == frameType))) {
            frameType = pFrameToImageInfo.frameType;
            return pFrameToImageInfo.imageResource;
        }
    }

    return QStringLiteral("AirframeUnknown");
}

QString APMFrameClass::_findImageResourceRover(int frameClass, int frameType)
{
    Q_UNUSED(frameType);

    static const QList<FrameToImageInfo> s_rgFrameToImageRover = {
        { FRAME_CLASS_ROVER, -1, "Rover" },
        { FRAME_CLASS_BOAT,  -1, "Boat" },
    };

    for (const FrameToImageInfo &pFrameToImageInfo : s_rgFrameToImageRover) {
        if (pFrameToImageInfo.frameClass == frameClass) {
            return pFrameToImageInfo.imageResource;
        }
    }

    return QStringLiteral("AirframeUnknown");
}
