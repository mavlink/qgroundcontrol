#pragma once

#include <QtCore/QByteArray>

#include <memory>
#include <optional>
#include <utility>

#include "NMEASentence.h"

/// Parsed views share ownership of their immutable sentence bytes across queued consumers.
class NMEASentenceEnvelope
{
public:
    static std::optional<NMEASentenceEnvelope> parse(QByteArray bytes, quint64 receivedAtUs)
    {
        auto data = std::make_shared<Data>();
        data->bytes = std::move(bytes);
        const auto sentence = NMEA::sentence({data->bytes.constData(), static_cast<size_t>(data->bytes.size())});
        if (!sentence) {
            return std::nullopt;
        }
        data->sentence = *sentence;
        data->receivedAtUs = receivedAtUs;
        return NMEASentenceEnvelope(std::move(data));
    }

    const QByteArray& bytes() const { return _data->bytes; }

    const NMEA::Sentence& sentence() const { return _data->sentence; }

    quint64 receivedAtUs() const { return _data->receivedAtUs; }

private:
    struct Data
    {
        QByteArray bytes;
        NMEA::Sentence sentence;
        quint64 receivedAtUs = 0;
    };

    explicit NMEASentenceEnvelope(std::shared_ptr<const Data> data)
        : _data(std::move(data))
    {}

    std::shared_ptr<const Data> _data;
};

/// Supplies the envelope belonging to the last line read by Qt's position decoder.
class NMEASentenceProvider
{
public:
    virtual ~NMEASentenceProvider() = default;
    virtual std::optional<NMEASentenceEnvelope> lastReadSentence() const = 0;
};
