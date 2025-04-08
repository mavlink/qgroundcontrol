/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QLoggingCategory>
#include <QtCore/QObject>

#include "FactPanelController.h"

class APMAirframeModel;
class APMAirframeType;
class QmlObjectListModel;

Q_DECLARE_LOGGING_CATEGORY(APMAirframeComponentControllerLog)

/// MVC Controller for APMAirframeComponent.qml.
class APMAirframeComponentController : public FactPanelController
{
    Q_OBJECT
    Q_MOC_INCLUDE("QmlObjectListModel.h")
    Q_PROPERTY(QmlObjectListModel *frameClassModel MEMBER _frameClassModel CONSTANT)

public:
    explicit APMAirframeComponentController(QObject *parent = nullptr);
    ~APMAirframeComponentController();


    Q_INVOKABLE void loadParameters(const QString &paramFile);

private slots:
    void _githubJsonDownloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);
    void _paramFileDownloadComplete(const QString &remoteFile, const QString &localFile, const QString &errorMsg);

private:
    void _fillFrameClasses();
    void _loadParametersFromDownloadFile(const QString &downloadedParamFile);

    Fact *_frameClassFact = nullptr;
    Fact *_frameTypeFact = nullptr;
    QmlObjectListModel *_frameClassModel = nullptr;
};

/*===========================================================================*/

class APMFrameClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString      name                    MEMBER _name                    CONSTANT)
    Q_PROPERTY(int          frameClass              MEMBER _frameClass              CONSTANT)
    Q_PROPERTY(int          frameType               READ   frameType                NOTIFY frameTypeChanged)
    Q_PROPERTY(QStringList  frameTypeEnumStrings    MEMBER _frameTypeEnumStrings    CONSTANT)
    Q_PROPERTY(QVariantList frameTypeEnumValues     MEMBER _frameTypeEnumValues     CONSTANT)
    Q_PROPERTY(int          defaultFrameType        MEMBER _defaultFrameType        CONSTANT)
    Q_PROPERTY(QString      imageResource           READ   imageResource            NOTIFY imageResourceChanged)
    Q_PROPERTY(QString      imageResourceDefault    MEMBER _imageResourceDefault    CONSTANT)
    Q_PROPERTY(bool         frameTypeSupported      MEMBER _frameTypeSupported      CONSTANT)

public:
    explicit APMFrameClass(const QString &name, bool copter, int frameClass, Fact *frameTypeFact, QObject *parent = nullptr);
    ~APMFrameClass();

    int frameType() const;
    QString imageResource() const;

    const QString _name;
    const bool _copter;
    QString _imageResource;
    QString _imageResourceDefault;
    const int _frameClass;
    QStringList _frameTypeEnumStrings;
    QVariantList _frameTypeEnumValues;
    int _defaultFrameType = -1;
    bool _frameTypeSupported = false;

signals:
    void imageResourceChanged();
    void frameTypeChanged();

private:
    /// Returns the image resource for the frameClass, frameType pair
    ///     @param[in,out] frameType Specified frame type, or -1 to match first item in list (frameType found will be returned)
    static QString _findImageResourceCopter(int frameClass, int &frameType);
    static QString _findImageResourceRover(int frameClass, int frameType);

    const Fact *_frameTypeFact = nullptr;

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

    struct FrameToImageInfo {
        const int frameClass;
        const int frameType;
        const char *imageResource;
    };
    static constexpr const FrameToImageInfo _rgFrameToImageCopter[] = {
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
};
