/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

    Q_PROPERTY(double       complexDistance     READ complexDistance    NOTIFY complexDistanceChanged)
    Q_PROPERTY(bool         presetsSupported    READ presetsSupported   CONSTANT)
    Q_PROPERTY(QStringList  presetNames         READ presetNames        NOTIFY presetNamesChanged)
    Q_PROPERTY(bool         isIncomplete        READ isIncomplete       NOTIFY isIncompleteChanged)

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

     Q_INVOKABLE void deletePreset(const QString& name);


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

    bool presetsSupported   (void) { return !presetsSettingsGroup().isEmpty(); }
    bool isIncomplete       (void) const { return _isIncomplete; }

    /// This mission item attribute specifies the type of the complex item.
    static const char* jsonComplexItemTypeKey;

signals:
    void complexDistanceChanged     (void);
    void boundingCubeChanged        (void);
    void greatestDistanceToChanged  (void);
    void presetNamesChanged         (void);
    void isIncompleteChanged        (void);

protected:
    void        _savePresetJson (const QString& name, QJsonObject& presetObject);
    QJsonObject _loadPresetJson (const QString& name);

    bool _isIncomplete = true;

    QMap<QString, FactMetaData*> _metaDataMap;

    static const char* _presetSettingsKey;
};

#endif
