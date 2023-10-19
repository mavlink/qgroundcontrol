/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include "QGCMapCircle.h"

/// The QGCFenceCircle class provides a cicle used by GeoFence support.
class QGCFenceCircle : public QGCMapCircle
{
    Q_OBJECT

public:
    QGCFenceCircle(QObject* parent = nullptr);
    QGCFenceCircle(const QGeoCoordinate& center, double radius, bool inclusion, QObject* parent = nullptr);
    QGCFenceCircle(const QGCFenceCircle& other, QObject* parent = nullptr);

    const QGCFenceCircle& operator=(const QGCFenceCircle& other);

    Q_PROPERTY(bool inclusion READ inclusion WRITE setInclusion NOTIFY inclusionChanged)

    /// Saves the QGCFenceCircle to the json object.
    ///     @param json Json object to save to
    void saveToJson(QJsonObject& json);

    /// Load a QGCFenceCircle from json
    ///     @param json Json object to load from
    ///     @param errorString Error string if return is false
    /// @return true: success, false: failure (errorString set)
    bool loadFromJson(const QJsonObject& json, QString& errorString);

    // Property methods

    bool inclusion      (void) const { return _inclusion; }
    int  fenceAction    (void) const { return _fenceAction; }
    void setInclusion   (bool inclusion);
    void setFenceAction (int fenceAction);

signals:
    void inclusionChanged(bool inclusion);

private slots:
    void _setDirty(void);

private:
    constexpr static int DEFAULT_FENCE_ACTION = 2; // HOLD
    void _init(void);

    bool _inclusion;
    int _fenceAction{DEFAULT_FENCE_ACTION};

    static const int _jsonCurrentVersion = 1;

    static const char* _jsonInclusionKey;
    static const char* _jsonFenceActionKey;
};
