/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "APMAirframeComponentController.h"
#include "QGCMAVLink.h"
#include "MultiVehicleManager.h"
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "ParameterManager.h"
#include "ArduCopterFirmwarePlugin.h"
#include "ArduRoverFirmwarePlugin.h"

#include <QVariant>
#include <QQmlProperty>
#include <QStandardPaths>
#include <QDir>
#include <QJsonParseError>
#include <QJsonObject>

// These should match the ArduCopter FRAME_CLASS parameter enum meta data
#define FRAME_CLASS_UNDEFINED       0
#define FRAME_CLASS_QUAD            1
#define FRAME_CLASS_HEX             2
#define FRAME_CLASS_OCTA            3
#define FRAME_CLASS_OCTAQUAD        4
#define FRAME_CLASS_Y6              5
#define FRAME_CLASS_HELI            6
#define FRAME_CLASS_TRI             7
#define FRAME_CLASS_SINGLECOPTER    8
#define FRAME_CLASS_COAXCOPTER      9
#define FRAME_CLASS_BICOPTER        10
#define FRAME_CLASS_HELI_DUAL       11
#define FRAME_CLASS_DODECAHEXA      12
#define FRAME_CLASS_HELIQUAD        13

// These should match the ArduCopter FRAME_TYPE parameter enum meta data
#define FRAME_TYPE_PLUS         0
#define FRAME_TYPE_X            1
#define FRAME_TYPE_V            2
#define FRAME_TYPE_H            3
#define FRAME_TYPE_V_TAIL       4
#define FRAME_TYPE_A_TAIL       5
#define FRAME_TYPE_Y6B          10
#define FRAME_TYPE_Y6F          11
#define FRAME_TYPE_BETAFLIGHTX  12
#define FRAME_TYPE_DJIX         13
#define FRAME_TYPE_CLOCKWISEX   14

// These should match the Rover FRAME_CLASS parameter enum meta data
#define FRAME_CLASS_ROVER       1
#define FRAME_CLASS_BOAT        2
#define FRAME_CLASS_BALANCEBOT  3

// These should match the Rover FRAME_TYPE parameter enum meta data
#define FRAME_TYPE_UNDEFINED    0
#define FRAME_TYPE_OMNI3        1
#define FRAME_TYPE_OMNIX        2
#define FRAME_TYPE_OMNIPLUS     3

typedef struct {
    int         frameClass;
    int         frameType;
    const char* imageResource;
} FrameToImageInfo_t;

static const FrameToImageInfo_t s_rgFrameToImageCopter[] = {
    { FRAME_CLASS_QUAD,         FRAME_TYPE_X,       "QuadRotorX" },             // Default
    { FRAME_CLASS_QUAD,         FRAME_TYPE_PLUS,    "QuadRotorPlus" },
    { FRAME_CLASS_QUAD,         FRAME_TYPE_V,       "QuadRotorWide" },
    { FRAME_CLASS_QUAD,         FRAME_TYPE_H,       "QuadRotorH" },
    { FRAME_CLASS_QUAD,         FRAME_TYPE_V_TAIL,  "QuadRotorVTail" },
    { FRAME_CLASS_QUAD,         FRAME_TYPE_A_TAIL,  "QuadRotorATail" },

    { FRAME_CLASS_HEX,          FRAME_TYPE_X,       "HexaRotorX" },             // Default
    { FRAME_CLASS_HEX,          FRAME_TYPE_PLUS,    "HexaRotorPlus" },

    { FRAME_CLASS_OCTA,         FRAME_TYPE_X,       "OctoRotorX" },             // Default
    { FRAME_CLASS_OCTA,         FRAME_TYPE_PLUS,    "OctoRotorPlus" },
    { FRAME_CLASS_OCTA,         FRAME_TYPE_V,       "AirframeUnknown" },
    { FRAME_CLASS_OCTA,         FRAME_TYPE_H,       "AirframeUnknown" },

    { FRAME_CLASS_OCTAQUAD,     FRAME_TYPE_X,       "OctoRotorXCoaxial" },      // Default
    { FRAME_CLASS_OCTAQUAD,     FRAME_TYPE_PLUS,    "OctoRotorPlusCoaxial" },
    { FRAME_CLASS_OCTAQUAD,     FRAME_TYPE_V,       "AirframeUnknown" },
    { FRAME_CLASS_OCTAQUAD,     FRAME_TYPE_H,       "AirframeUnknown" },

    { FRAME_CLASS_Y6,           FRAME_TYPE_Y6B,     "Y6B" },                    // Default
    { FRAME_CLASS_Y6,           FRAME_TYPE_Y6F,     "AirframeUnknown" },
    { FRAME_CLASS_Y6,           -1,                 "Y6A" },

    { FRAME_CLASS_DODECAHEXA,   FRAME_TYPE_X,       "AirframeUnknown" },        // Default
    { FRAME_CLASS_DODECAHEXA,   FRAME_TYPE_PLUS,    "AirframeUnknown" },

    { FRAME_CLASS_HELI,         -1,                 "Helicopter" },
    { FRAME_CLASS_TRI,          -1,                 "YPlus" },
};

static const FrameToImageInfo_t s_rgFrameToImageRover[] = {
    { FRAME_CLASS_ROVER,    -1, "Rover" },
    { FRAME_CLASS_BOAT,     -1, "Boat" },
};

/// Returns the image resource for the frameClass, frameType pair
///     @param[in,out] frameType Specified frame type, or -1 to match first item in list (frameType found will be returned)
static QString s_findImageResourceCopter(int frameClass, int& frameType)
{
    for (size_t i=0; i<sizeof(s_rgFrameToImageCopter)/sizeof(s_rgFrameToImageCopter[0]); i++) {
        const FrameToImageInfo_t* pFrameToImageInfo = &s_rgFrameToImageCopter[i];

        if ((pFrameToImageInfo->frameClass == frameClass && frameType == -1) ||
                (pFrameToImageInfo->frameClass == frameClass && pFrameToImageInfo->frameType == frameType)) {
            frameType = pFrameToImageInfo->frameType;
            return pFrameToImageInfo->imageResource;
        }
    }

    return QStringLiteral("AirframeUnknown");
}

static QString s_findImageResourceRover(int frameClass, int frameType)
{
    Q_UNUSED(frameType);

    for (size_t i=0; i<sizeof(s_rgFrameToImageRover)/sizeof(s_rgFrameToImageRover[0]); i++) {
        const FrameToImageInfo_t* pFrameToImageInfo = &s_rgFrameToImageRover[i];

        if (pFrameToImageInfo->frameClass == frameClass) {
            return pFrameToImageInfo->imageResource;
        }
    }

    return QStringLiteral("AirframeUnknown");
}

APMAirframeComponentController::APMAirframeComponentController(void)
    : _frameClassFact   (getParameterFact(FactSystem::defaultComponentId, QStringLiteral("FRAME_CLASS"), false /* reportMissing */))
    , _frameTypeFact    (getParameterFact(FactSystem::defaultComponentId, QStringLiteral("FRAME_TYPE"), false /* reportMissing */))
    , _frameClassModel  (new QmlObjectListModel(this))
{
    _fillFrameClasses();
}

APMAirframeComponentController::~APMAirframeComponentController()
{

}

void APMAirframeComponentController::_fillFrameClasses()
{
    FirmwarePlugin* fwPlugin = _vehicle->firmwarePlugin();

    if (qobject_cast<ArduCopterFirmwarePlugin*>(fwPlugin)) {
        QList<int> frameTypeNotSupported;

        frameTypeNotSupported << FRAME_CLASS_HELI
                              << FRAME_CLASS_SINGLECOPTER
                              << FRAME_CLASS_COAXCOPTER
                              << FRAME_CLASS_BICOPTER
                              << FRAME_CLASS_HELI_DUAL
                              << FRAME_CLASS_HELIQUAD;

        for (int i=1; i<_frameClassFact->enumStrings().count(); i++) {
            QString frameClassName =    _frameClassFact->enumStrings()[i];
            int     frameClass =        _frameClassFact->enumValues()[i].toInt();

            if (frameClass == FRAME_CLASS_HELI) {
                // Heli requires it's own firmware variant. You can't switch to Heli from a Copter variant firmware.
                continue;
            }

            _frameClassModel->append(new APMFrameClass(frameClassName, true /* copter */, frameClass, _frameTypeFact, _frameClassModel));
        }
    } else if (qobject_cast<ArduRoverFirmwarePlugin*>(fwPlugin)) {
        for (int i=1; i<_frameClassFact->enumStrings().count(); i++) {
            QString frameClassName =    _frameClassFact->enumStrings()[i];
            int     frameClass =        _frameClassFact->enumValues()[i].toInt();
            _frameClassModel->append(new APMFrameClass(frameClassName, false /* copter */, frameClass, _frameTypeFact, _frameClassModel));
        }
    }
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

void APMAirframeComponentController::loadParameters(const QString& paramFile)
{
    qgcApp()->setOverrideCursor(Qt::WaitCursor);

    QString paramFileUrl = QStringLiteral("https://api.github.com/repos/ArduPilot/ardupilot/contents/Tools/Frame_params/%1?ref=master");

    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadComplete, this, &APMAirframeComponentController::_githubJsonDownloadComplete);
    downloader->download(paramFileUrl.arg(paramFile));
}

void APMAirframeComponentController::_githubJsonDownloadComplete(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    if (errorMsg.isEmpty()) {
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
        connect(downloader, &QGCFileDownload::downloadComplete, this, &APMAirframeComponentController::_paramFileDownloadComplete);
        downloader->download(json[QLatin1String("download_url")].toString());
    } else {
        qgcApp()->showAppMessage(tr("Param file github json download failed: %1").arg(errorMsg));
        qgcApp()->restoreOverrideCursor();
    }
}

void APMAirframeComponentController::_paramFileDownloadComplete(QString /*remoteFile*/, QString localFile, QString errorMsg)
{
    if (errorMsg.isEmpty()) {
        _loadParametersFromDownloadFile(localFile);
    } else {
        qgcApp()->showAppMessage(tr("Param file download failed: %1").arg(errorMsg));
        qgcApp()->restoreOverrideCursor();
    }
}

APMFrameClass::APMFrameClass(const QString& name, bool copter, int frameClass, Fact* frameTypeFact, QObject* parent)
    : QObject               (parent)
    , _name                 (name)
    , _copter               (copter)
    , _frameClass           (frameClass)
    , _defaultFrameType     (-1)
    , _frameTypeSupported   (false)
    , _frameTypeFact        (frameTypeFact)
{
    if (frameTypeFact) {
        connect(frameTypeFact, &Fact::rawValueChanged, this, &APMFrameClass::imageResourceChanged);
        connect(frameTypeFact, &Fact::rawValueChanged, this, &APMFrameClass::frameTypeChanged);
    }

    if (copter) {
        QList<int> rgSupportedFrameTypes;

        for (size_t i=0; i<sizeof(s_rgFrameToImageCopter)/sizeof(s_rgFrameToImageCopter[0]); i++) {
            const FrameToImageInfo_t* pFrameToImageInfo = &s_rgFrameToImageCopter[i];

            if (pFrameToImageInfo->frameClass == frameClass) {
                if (_defaultFrameType == -1) {
                    // Default frame type/icon is the first item found to match frameClass
                    _defaultFrameType = pFrameToImageInfo->frameType;
                    _imageResourceDefault = QStringLiteral("/qmlimages/Airframe/%1").arg(pFrameToImageInfo->imageResource);
                }

                if (pFrameToImageInfo->frameType != -1) {
                    // The list includes the supported frame types for the class
                    rgSupportedFrameTypes.append(pFrameToImageInfo->frameType);
                }
            }
        }
        if (_imageResourceDefault.isEmpty()) {
            _imageResourceDefault = QStringLiteral("/qmlimages/Airframe/AirframeUnknown");
        }

        // Filter the enums
        for (const int frameType: rgSupportedFrameTypes) {
            int index = frameTypeFact->enumValues().indexOf(frameType);
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

QString APMFrameClass::imageResource(void)
{
    QString imageResource;

    int frameType = _frameTypeFact ? _frameTypeFact->rawValue().toInt() : -1;

    if (_copter) {
        imageResource = s_findImageResourceCopter(_frameClass, frameType);
    } else {
        imageResource = s_findImageResourceRover(_frameClass, frameType);
    }
    return QStringLiteral("/qmlimages/Airframe/%1").arg(imageResource);
}
