#pragma once

#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <memory>

class QIODevice;
class NMEAStreamDevice;

/// Copies one borrowed input stream into independent, bounded parser inputs.
/// The input and splitter must stay in the same thread.
class NMEAStreamSplitter : public QObject
{
    Q_OBJECT

public:
    explicit NMEAStreamSplitter(QIODevice* source, QObject* parent = nullptr);
    ~NMEAStreamSplitter() override;

    QIODevice* positionDevice() const;
    QIODevice* satelliteDevice() const;

private:
    void _readAvailableData();
    void _closeOutputs();

    QPointer<QIODevice> _source;
    std::unique_ptr<NMEAStreamDevice> _positionDevice;
    std::unique_ptr<NMEAStreamDevice> _satelliteDevice;
};
