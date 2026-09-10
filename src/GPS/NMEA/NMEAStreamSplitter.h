#pragma once

#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include <memory>

#include "NMEASentenceEnvelope.h"

class QIODevice;
class NMEAStreamDevice;

/// Frames one borrowed input into shared sentences and a bounded Qt position input.
/// The input and splitter must stay in the same thread.
class NMEAStreamSplitter : public QObject
{
    Q_OBJECT

public:
    explicit NMEAStreamSplitter(QIODevice* source, QObject* parent = nullptr);
    ~NMEAStreamSplitter() override;

    QIODevice* positionDevice() const;

signals:
    void sentenceReceived(const NMEASentenceEnvelope& sentence);
    void closed();

private:
    void _readAvailableData();
    void _closeOutputs();

    QPointer<QIODevice> _source;
    QByteArray _sentence;
    quint64 _sentenceTimestampUs = 0;
    bool _drainPending = false;
    std::unique_ptr<NMEAStreamDevice> _positionDevice;
};
