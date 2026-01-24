/// @file FuzzJsonParser.cc
/// @brief Fuzz test harness for JSON parsing
///
/// This fuzzer tests JSON parsing robustness by feeding random/mutated
/// data to the JSON parser. It helps find crashes, hangs, and undefined
/// behavior when processing malformed JSON.

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>

#include <cstdint>
#include <cstddef>

/// libFuzzer entry point
/// @param data Fuzz input data
/// @param size Size of input data
/// @return 0 on success (required by libFuzzer)
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    // Skip empty inputs
    if (size == 0) {
        return 0;
    }

    // Create QByteArray from fuzz input
    const QByteArray jsonData(reinterpret_cast<const char*>(data), static_cast<int>(size));

    // Attempt to parse as JSON
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);

    // If parsing succeeded, try to access the document
    if (error.error == QJsonParseError::NoError) {
        if (doc.isObject()) {
            const QJsonObject obj = doc.object();
            // Access all keys to exercise the parser
            const QStringList keys = obj.keys();
            for (const QString& key : keys) {
                (void) obj.value(key);
            }
        } else if (doc.isArray()) {
            const QJsonArray arr = doc.array();
            for (int i = 0; i < arr.size(); ++i) {
                (void) arr.at(i);
            }
        }
    }

    return 0;
}
