#pragma once

#include <QtCore/QByteArray>
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
    QByteArray _sentence;
    quint64 _sentenceTimestampUs = 0;
    bool _drainPending = false;
    std::unique_ptr<NMEAStreamDevice> _positionDevice;
    std::unique_ptr<NMEAStreamDevice> _satelliteDevice;
};
