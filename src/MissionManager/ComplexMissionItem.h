/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#ifndef ComplexMissionItem_H
#define ComplexMissionItem_H

#include "VisualMissionItem.h"
#include "QGCGeo.h"

#include <QSettings>

class ComplexMissionItem : public VisualMissionItem
{
    Q_OBJECT

public:
    ComplexMissionItem(Vehicle* vehicle, bool flyView, QObject* parent);

    const ComplexMissionItem& operator=(const ComplexMissionItem& other);

    Q_PROPERTY(double       complexDistance     READ complexDistance                            NOTIFY complexDistanceChanged)
    Q_PROPERTY(bool         presetsSupported    READ presetsSupported                           CONSTANT)
    Q_PROPERTY(QStringList  presetNames         READ presetNames                                NOTIFY presetNamesChanged)
    Q_PROPERTY(QString      currentPreset       READ currentPreset                              NOTIFY currentPresetChanged)
    Q_PROPERTY(bool         cameraInPreset      READ cameraInPreset     WRITE setCameraInPreset NOTIFY cameraInPresetChanged)
    Q_PROPERTY(bool         builtInPreset       READ builtInPreset      WRITE setBuiltInPreset  NOTIFY builtInPresetChanged)

    /// @return The distance covered the complex mission item in meters.
    /// Signals complexDistanceChanged
    virtual double complexDistance(void) const = 0;

    /// Load the complex mission item from Json
    ///     @param complexObject Complex mission item json object
    ///     @param sequenceNumber Sequence number for first MISSION_ITEM in survey
    ///     @param[out] errorString Error if load fails
    /// @return true: load success, false: load failed, errorString set
    virtual bool load(const QJsonObject& complexObject, int sequenceNumber, QString& errorString) = 0;

    /// Loads the specified preset into the complex item.
    ///     @param name Preset name.
    Q_INVOKABLE virtual void loadPreset(const QString& name);

    /// Saves the current state of the complex item as the named preset.
    ///     @param name User visible name for preset. Will replace existing preset if already exists.
    Q_INVOKABLE virtual void savePreset(const QString& name);

    Q_INVOKABLE void clearCurrentPreset(void);
    Q_INVOKABLE void deleteCurrentPreset(void);

    /// Get the point of complex mission item furthest away from a coordinate
    ///     @param other QGeoCoordinate to which distance is calculated
    /// @return the greatest distance from any point of the complex item to some coordinate
    /// Signals greatestDistanceToChanged
    virtual double greatestDistanceTo(const QGeoCoordinate &other) const = 0;

    /// Returns the list of currently saved presets for this complex item type.
    ///     @param name User visible name for preset. Will replace existing preset if already exists.
    virtual QStringList presetNames(void);

    /// Returns the name of the settings group for presets.
    ///     Empty string signals no support for presets.
    virtual QString presetsSettingsGroup(void) { return QString(); }

    bool    presetsSupported    (void) { return !presetsSettingsGroup().isEmpty(); }
    QString currentPreset       (void) const { return _currentPreset; }
    bool    cameraInPreset      (void) const { return _cameraInPreset; }
    bool    builtInPreset       (void) const { return _builtInPreset; }
    void    setCameraInPreset   (bool cameraInPreset);
    void    setBuiltInPreset    (bool builtInPreset);

    /// This mission item attribute specifies the type of the complex item.
    static const char* jsonComplexItemTypeKey;

signals:
    void complexDistanceChanged     (void);
    void boundingCubeChanged        (void);
    void greatestDistanceToChanged  (void);
    void presetNamesChanged         (void);
    void currentPresetChanged       (QString currentPreset);
    void cameraInPresetChanged      (bool cameraInPreset);
    void builtInPresetChanged       (bool builtInPreset);

protected:
    void        _saveItem       (QJsonObject& saveObject);
    void        _loadItem       (const QJsonObject& saveObject);
    void        _savePresetJson (const QString& name, QJsonObject& presetObject);
    QJsonObject _loadPresetJson (const QString& name);


    QMap<QString, FactMetaData*> _metaDataMap;

    QString         _currentPreset;
    SettingsFact    _saveCameraInPresetFact;
    bool            _cameraInPreset;
    bool            _builtInPreset;

    static const char* _presetSettingsKey;
    static const char* _presetNameKey;
    static const char* _saveCameraInPresetKey;
    static const char* _builtInPresetKey;
};

#endif
