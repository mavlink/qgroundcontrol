#pragma once

#include <QtCore/QObject>
#include <QtQmlIntegration/QtQmlIntegration>

class QGCFencePolygon;

/// \brief A single imported exclusion polygon awaiting operator review, plus its approval state.
class StagedExclusionZone : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Created by ExclusionZoneController")
    Q_MOC_INCLUDE("QGCFencePolygon.h")

    Q_PROPERTY(QGCFencePolygon* polygon READ polygon CONSTANT)
    Q_PROPERTY(bool approved READ approved WRITE setApproved NOTIFY approvedChanged)

public:
    /// \param polygon Owned by this object (parented to it).
    explicit StagedExclusionZone(QGCFencePolygon* polygon, QObject* parent = nullptr);

    QGCFencePolygon* polygon() const { return _polygon; }

    bool approved() const { return _approved; }

    void setApproved(bool approved);

signals:
    void approvedChanged(bool approved);

private:
    QGCFencePolygon* _polygon = nullptr;
    bool _approved = false;
};
