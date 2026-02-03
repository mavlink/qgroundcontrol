#include "Viewer3DInstancing.h"

#include "QGCLoggingCategory.h"

#include <cmath>

QGC_LOGGING_CATEGORY(Viewer3DInstancingLog, "Viewer3d.Viewer3DInstancing")

Viewer3DInstancing::Viewer3DInstancing(QQuick3DObject *parent)
    : QQuick3DInstancing(parent)
{
}

void Viewer3DInstancing::clear()
{
    qCDebug(Viewer3DInstancingLog) << "Clearing" << _entries.size() << "entries";
    _entries.clear();
    _dirty = true;
    markDirty();
    emit countChanged();
}

void Viewer3DInstancing::addEntry(const QVector3D &position,
                                  const QVector3D &scale,
                                  const QQuaternion &rotation,
                                  const QColor &color)
{
    _entries.append({position, scale, rotation, color});
    _dirty = true;
    markDirty();
    emit countChanged();
}

void Viewer3DInstancing::addLineSegment(const QVector3D &p1,
                                        const QVector3D &p2,
                                        float lineWidth,
                                        const QColor &color)
{
    const QVector3D delta = p2 - p1;
    const float distance = delta.length();
    if (distance < 1e-6f) {
        qCDebug(Viewer3DInstancingLog) << "Skipping degenerate line segment (length < 1e-6)";
        return;
    }

    // #Cylinder: 100 units tall on Y, radius 50
    const float radius = lineWidth * 0.1f;
    const QVector3D position = (p1 + p2) * 0.5f;
    const QVector3D scale(radius / 50.0f, distance / 100.0f, radius / 50.0f);

    const QVector3D yAxis(0.0f, 1.0f, 0.0f);
    const QQuaternion rotation = QQuaternion::rotationTo(yAxis, delta.normalized());

    _entries.append({position, scale, rotation, color});
    _dirty = true;
    markDirty();
    emit countChanged();
}

int Viewer3DInstancing::count() const
{
    return _entries.size();
}

int Viewer3DInstancing::selectedIndex() const
{
    return _selectedIndex;
}

void Viewer3DInstancing::setSelectedIndex(int index)
{
    if (_selectedIndex == index) return;
    _selectedIndex = index;
    _dirty = true;
    markDirty();
    emit selectedIndexChanged();
}

QByteArray Viewer3DInstancing::getInstanceBuffer(int *instanceCount)
{
    if (_dirty) {
        _instanceData.clear();
        for (int i = 0; i < _entries.size(); ++i) {
            const auto &entry = _entries[i];
            const QColor color = (i == _selectedIndex) ? kHighlightColor : entry.color;
            auto tableEntry = calculateTableEntry(entry.position, entry.scale, entry.rotation.toEulerAngles(), color);
            _instanceData.append(reinterpret_cast<const char *>(&tableEntry), sizeof(tableEntry));
        }
        _dirty = false;
    }

    if (instanceCount) {
        *instanceCount = _entries.size();
    }
    return _instanceData;
}
