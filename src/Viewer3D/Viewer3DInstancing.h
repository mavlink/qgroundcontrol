#pragma once

#include <QtCore/QLoggingCategory>
#include <QtGui/QColor>
#include <QtGui/QQuaternion>
#include <QtGui/QVector3D>
#include <QtQmlIntegration/QtQmlIntegration>
#include <QtQuick3D/QQuick3DInstancing>

Q_DECLARE_LOGGING_CATEGORY(Viewer3DInstancingLog)

class Viewer3DInstancing : public QQuick3DInstancing
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)

    friend class Viewer3DInstancingTest;

public:
    explicit Viewer3DInstancing(QQuick3DObject *parent = nullptr);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void addEntry(const QVector3D &position,
                              const QVector3D &scale,
                              const QQuaternion &rotation,
                              const QColor &color);
    Q_INVOKABLE void addLineSegment(const QVector3D &p1,
                                    const QVector3D &p2,
                                    float lineWidth,
                                    const QColor &color);
    int count() const;

    int selectedIndex() const;
    void setSelectedIndex(int index);

signals:
    void countChanged();
    void selectedIndexChanged();

protected:
    QByteArray getInstanceBuffer(int *instanceCount) override;

private:
    struct InstanceEntry {
        QVector3D position;
        QVector3D scale;
        QQuaternion rotation;
        QColor color;
    };

    static inline const QColor kHighlightColor{255, 255, 0};

    QList<InstanceEntry> _entries;
    QByteArray _instanceData;
    bool _dirty = false;
    int _selectedIndex = -1;
};
