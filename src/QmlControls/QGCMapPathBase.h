#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtCore/QPointF>
#include <QtCore/QVariantList>
#include <QtPositioning/QGeoCoordinate>

#include "QmlObjectListModel.h"

class QGCMapPathBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QVariantList path READ path NOTIFY pathChanged)
    Q_PROPERTY(QmlObjectListModel* pathModel READ qmlPathModel CONSTANT)
    Q_PROPERTY(bool dirty READ dirty WRITE setDirty NOTIFY dirtyChanged)
    Q_PROPERTY(bool interactive READ interactive WRITE setInteractive NOTIFY interactiveChanged)
    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged)
    Q_PROPERTY(bool empty READ empty NOTIFY isEmptyChanged)
    Q_PROPERTY(bool traceMode READ traceMode WRITE setTraceMode NOTIFY traceModeChanged)
    Q_PROPERTY(int selectedVertex READ selectedVertex WRITE selectVertex NOTIFY selectedVertexChanged)

public:
    explicit QGCMapPathBase(QObject* parent = nullptr);
    QGCMapPathBase(const QGCMapPathBase& other, QObject* parent = nullptr);

    ~QGCMapPathBase() override;

    const QGCMapPathBase& operator=(const QGCMapPathBase& other);

    Q_INVOKABLE void clear();
    Q_INVOKABLE void appendVertex(const QGeoCoordinate& coordinate);
    Q_INVOKABLE void removeVertex(int vertexIndex);
    Q_INVOKABLE virtual void adjustVertex(int vertexIndex, const QGeoCoordinate coordinate);
    Q_INVOKABLE void splitSegment(int vertexIndex);
    Q_INVOKABLE bool loadShapeFile(const QString& file);
    Q_INVOKABLE QGeoCoordinate vertexCoordinate(int vertex) const;
    Q_INVOKABLE void beginReset();
    Q_INVOKABLE void endReset();

    void appendVertices(const QList<QGeoCoordinate>& coordinates);
    QList<QGeoCoordinate> coordinateList() const;

    void saveToJson(QJsonObject& json);
    bool loadFromJson(const QJsonObject& json, bool required, QString& errorString);

    QList<QPointF> nedPath() const;

    int count() const { return _path.count(); }

    bool dirty() const { return _dirty; }

    void setDirty(bool dirty);

    bool interactive() const { return _interactive; }

    bool isValid() const { return _model.count() >= _minVertexCount(); }

    bool empty() const { return _model.count() == 0; }

    bool traceMode() const { return _traceMode; }

    int selectedVertex() const { return _selectedVertexIndex; }

    QVariantList path() const { return _path; }

    QmlObjectListModel* qmlPathModel() { return &_model; }

    QmlObjectListModel& pathModel() { return _model; }

    void setPath(const QList<QGeoCoordinate>& path);
    void setPath(const QVariantList& path);
    void setInteractive(bool interactive);
    void setTraceMode(bool traceMode);
    void selectVertex(int index);

signals:
    void countChanged(int count);
    void pathChanged();
    void dirtyChanged(bool dirty);
    void cleared();
    void interactiveChanged(bool interactive);
    void isValidChanged();
    void isEmptyChanged();
    void traceModeChanged(bool traceMode);
    void selectedVertexChanged(int index);

protected:
    virtual int _minVertexCount() const = 0;

    virtual bool _closedPath() const { return false; }

    virtual void _onPathChanged() {}

    virtual void _onModelReset();
    virtual bool _loadFromFile(const QString& file, QList<QList<QGeoCoordinate>>& coords, QString& errorString) = 0;
    virtual const char* _jsonKey() const = 0;
    virtual void _clearImpl();
    virtual QString _emptyFileMessage() const = 0;

    QGeoCoordinate _coordFromPointF(const QPointF& point) const;
    QPointF _pointFFromCoord(const QGeoCoordinate& coordinate) const;

    QVariantList _path;
    QmlObjectListModel _model;
    bool _deferredPathChanged = false;
    bool _dirty = false;
    bool _interactive = false;
    bool _traceMode = false;
    int _selectedVertexIndex = -1;

private slots:
    void _modelCountChanged(int count);
    void _modelDirtyChanged(bool dirty);

private:
    void _init();
};
