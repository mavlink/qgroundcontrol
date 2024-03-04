#ifndef KLVDECODER_H
#define KLVDECODER_H

#include <unordered_map>
#include <optional>
#include <cstdint>
#include <vector>
#include <QString>

class KLVDecoder
{
public:
    KLVDecoder() = default;

    /*!
     * \brief Decode received metadata and store the result internally
     * \param data Raw data to be decoded
     * \param length Length of the data array
     */
    void decode(uint8_t *data, size_t length);

    /*!
     * \brief Get latest raw value of the specified key
     * \return The value corresponding to the key if it was received during last decode invocatoin
     */
    std::optional<std::vector<uint8_t>> getRawKeyValue(uint32_t key);

    /*!
     * \brief Get the latest received timestamp (key = 2)
     * \return
     */
    std::optional<uint64_t> getTimestamp();

    /*!
     * \brief Get the latest received mission ID (key = 3)
     * \return
     */
    std::optional<QString> getMissionID();

    /*!
     * \brief Get the latest received image sensor (key = 11)
     * \return
     */
    std::optional<QString> getImageSensor();

    /*!
     * \brief Get all the parsed metadata in raw format of a mapping keys to values
     * \return
     */
    std::unordered_map<uint32_t, std::vector<uint8_t>> getAllMetadata();

private:

    std::unordered_map<uint32_t, std::vector<uint8_t>> _lastMetadata;

    uint64_t _decodeUint64(const std::vector<uint8_t> &value);

    QString _decodeString(const std::vector<uint8_t> &value);


};

#endif // KLVDECODER_H
