#pragma once

#include "QGCMapCircle.h"

/// \brief The QGCFenceCircle class provides a cicle used by GeoFence support.
///
class QGCFenceCircle : public QGCMapCircle
{
    Q_OBJECT

public:
    QGCFenceCircle(QObject* parent = nullptr);
    QGCFenceCircle(const QGeoCoordinate& center, double radius, bool inclusion, QObject* parent = nullptr);
    QGCFenceCircle(const QGCFenceCircle& other, QObject* parent = nullptr);

    const QGCFenceCircle& operator=(const QGCFenceCircle& other);

    Q_PROPERTY(bool inclusion READ inclusion WRITE setInclusion NOTIFY inclusionChanged)
    /// true: circle follows the vehicle home position (MAV_CMD_NAV_FENCE_HOME_CIRCLE_INCLUSION)
    Q_PROPERTY(bool homeCentered READ homeCentered WRITE setHomeCentered NOTIFY homeCenteredChanged)

    /// Saves the QGCFenceCircle to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a QGCFenceCircle from json
    ///     @param json Json object to load from
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, QString& errorString);

    // Property methods

    bool inclusion(void) const { return _inclusion; }

    void setInclusion(bool inclusion);

    bool homeCentered(void) const { return _homeCentered; }

    void setHomeCentered(bool homeCentered);

signals:
    void inclusionChanged(bool inclusion);
    void homeCenteredChanged(bool homeCentered);

private slots:
    void _setDirty(void);

private:
    void _init(void);

    bool _inclusion;
    bool _homeCentered = false;

    static constexpr int _jsonCurrentVersion = 1;
    /// Home-centered circles are written as version 2 so older readers reject them rather than
    /// loading them as a fixed circle at the saved center
    static constexpr int _jsonHomeCenteredVersion = 2;

    static constexpr const char* _jsonInclusionKey = "inclusion";
    static constexpr const char* _jsonHomeCenteredKey = "homeCentered";
};
