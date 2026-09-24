#pragma once

#include <optional>
#include <utility>

#include <QtCore/QByteArray>

#include "NMEASentence.h"

/// A parsed sentence with views into its own bytes. Copies share the immutable buffer, so views stay valid
/// across queued consumers.
class NMEASentenceEnvelope
{
public:
    static std::optional<NMEASentenceEnvelope> parse(QByteArray bytes, quint64 receivedAtUs)
    {
        // fromRawData() can borrow storage even after a move. Own it before creating views.
        bytes.detach();
        const auto sentence = NMEA::sentence({bytes.constData(), static_cast<size_t>(bytes.size())});
        if (!sentence) {
            return std::nullopt;
        }
        return NMEASentenceEnvelope(std::move(bytes), *sentence, receivedAtUs);
    }

    const QByteArray& bytes() const { return _bytes; }

    const NMEA::Sentence& sentence() const { return _sentence; }

    quint64 receivedAtUs() const { return _receivedAtUs; }

private:
    NMEASentenceEnvelope(QByteArray bytes, const NMEA::Sentence& sentence, quint64 receivedAtUs)
        : _bytes(std::move(bytes))
        , _sentence(sentence)
        , _receivedAtUs(receivedAtUs)
    {}

    // Moving or copying QByteArray keeps the shared data pointer that the sentence views reference.
    QByteArray _bytes;
    NMEA::Sentence _sentence;
    quint64 _receivedAtUs = 0;
};
