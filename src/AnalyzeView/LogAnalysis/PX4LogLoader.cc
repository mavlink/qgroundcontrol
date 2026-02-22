#include "PX4LogLoader.h"
#include "PX4LogTopic.h"
#include "QGCLoggingCategory.h"
#include "ULogUtility.h"

#include <ulog_cpp/data_container.hpp>
#include <ulog_cpp/reader.hpp>

#include <QtCore/QFile>

QGC_LOGGING_CATEGORY(PX4LogLoaderLog, "AnalyzeView.PX4LogLoader")

PX4LogLoader::PX4LogLoader(QObject *parent)
    : QObject(parent)
{
}

PX4LogLoader::~PX4LogLoader()
{
    clear();
}

void PX4LogLoader::clear()
{
    qDeleteAll(_topics);
    _topics.clear();
    _topicNames.clear();
    _parameters.clear();
    _infoMessages.clear();
    _logMessages.clear();
    _dataContainer.reset();
    _dropoutCount = 0;
    _durationUs = 0;

    if (_loaded) {
        _loaded = false;
        emit loadedChanged();
    }

    if (!_errorMessage.isEmpty()) {
        _errorMessage.clear();
        emit errorMessageChanged();
    }
}

bool PX4LogLoader::load(const QString &filePath)
{
    clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        _errorMessage = QStringLiteral("Failed to open file: ") + filePath;
        emit errorMessageChanged();
        return false;
    }

    const qint64 fileSize = file.size();
    if (fileSize < ULogUtility::kHeaderSize) {
        _errorMessage = QStringLiteral("File too small to be a valid ULog");
        emit errorMessageChanged();
        return false;
    }

    // Memory-map the file for efficient parsing of large logs
    const uchar *mappedData = file.map(0, fileSize);
    const char *data = nullptr;
    qint64 dataSize = fileSize;
    QByteArray fallbackBuffer;

    if (mappedData) {
        data = reinterpret_cast<const char *>(mappedData);
        qCDebug(PX4LogLoaderLog) << "Memory-mapped log file:" << fileSize << "bytes";
    } else {
        qCDebug(PX4LogLoaderLog) << "Memory mapping failed, reading file into memory";
        fallbackBuffer = file.readAll();
        if (fallbackBuffer.isEmpty()) {
            _errorMessage = QStringLiteral("Failed to read file into memory");
            emit errorMessageChanged();
            return false;
        }
        data = fallbackBuffer.constData();
        dataSize = fallbackBuffer.size();
    }

    if (!ULogUtility::isValidHeader(data, dataSize)) {
        _errorMessage = QStringLiteral("Not a valid ULog file (bad magic bytes)");
        emit errorMessageChanged();
        return false;
    }

    // Parse the full log into memory using ulog_cpp::DataContainer
    _dataContainer = std::make_shared<ulog_cpp::DataContainer>(ulog_cpp::DataContainer::StorageConfig::FullLog);
    ulog_cpp::Reader reader(_dataContainer);
    reader.readChunk(reinterpret_cast<const uint8_t *>(data), static_cast<size_t>(dataSize));

    if (_dataContainer->hadFatalError()) {
        _errorMessage = QStringLiteral("Fatal error parsing ULog file");
        const auto &errors = _dataContainer->parsingErrors();
        if (!errors.empty()) {
            _errorMessage += QStringLiteral(": ") + QString::fromStdString(errors.front());
        }
        emit errorMessageChanged();
        return false;
    }

    if (!_dataContainer->isHeaderComplete()) {
        _errorMessage = QStringLiteral("ULog header is incomplete or corrupt");
        emit errorMessageChanged();
        return false;
    }

    // Log any non-fatal parsing errors
    for (const auto &err : _dataContainer->parsingErrors()) {
        qCWarning(PX4LogLoaderLog) << "Parse warning:" << QString::fromStdString(err);
    }

    _extractMetadata();

    _loaded = true;
    emit loadedChanged();

    qCDebug(PX4LogLoaderLog) << "Loaded" << filePath
                             << "- topics:" << _topicNames.count()
                             << "params:" << _parameters.count()
                             << "log messages:" << _logMessages.count()
                             << "dropouts:" << _dropoutCount;
    return true;
}

PX4LogTopic *PX4LogLoader::topic(const QString &name, int multiId) const
{
    const QString key = (multiId > 0) ? (name + QStringLiteral("/") + QString::number(multiId)) : name;
    return _topics.value(key, nullptr);
}

void PX4LogLoader::_extractMetadata()
{
    // --- Subscriptions (topic data) ---
    const auto &subscriptions = _dataContainer->subscriptionsByNameAndMultiId();

    quint64 earliestTimestamp = std::numeric_limits<quint64>::max();
    quint64 latestTimestamp = 0;

    for (const auto &[nameAndId, sub] : subscriptions) {
        const QString topicName = QString::fromStdString(nameAndId.name);
        const int multiId = nameAndId.multi_id;
        const QString key = (multiId > 0) ? (topicName + QStringLiteral("/") + QString::number(multiId)) : topicName;

        auto *logTopic = new PX4LogTopic(nameAndId.name, sub, this);
        _topics.insert(key, logTopic);

        if (!_topicNames.contains(key)) {
            _topicNames.append(key);
        }

        // Track log duration from first/last timestamps
        if (sub->size() > 0) {
            try {
                const auto firstTs = (*sub->begin()).at("timestamp").as<uint64_t>();
                const auto lastTs = (*(sub->end() - 1)).at("timestamp").as<uint64_t>();
                earliestTimestamp = std::min(earliestTimestamp, firstTs);
                latestTimestamp = std::max(latestTimestamp, lastTs);
            } catch (const ulog_cpp::AccessException &) {
                // Some topics may not have a timestamp field
            }
        }
    }

    _topicNames.sort();

    if (latestTimestamp > earliestTimestamp) {
        _durationUs = latestTimestamp - earliestTimestamp;
    }

    // --- Parameters ---
    for (const auto &[name, param] : _dataContainer->initialParameters()) {
        try {
            const auto variant = param.value().asNativeTypeVariant();
            std::visit([&](auto &&val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_arithmetic_v<T>) {
                    _parameters.insert(QString::fromStdString(name), static_cast<double>(val));
                } else if constexpr (std::is_same_v<T, std::string>) {
                    _parameters.insert(QString::fromStdString(name), QString::fromStdString(val));
                }
            }, variant);
        } catch (const ulog_cpp::AccessException &e) {
            qCWarning(PX4LogLoaderLog) << "Failed to read parameter" << QString::fromStdString(name) << ":" << e.what();
        }
    }

    // --- Info messages ---
    for (const auto &[name, info] : _dataContainer->messageInfo()) {
        try {
            const auto variant = info.value().asNativeTypeVariant();
            std::visit([&](auto &&val) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_arithmetic_v<T>) {
                    _infoMessages.insert(QString::fromStdString(name), static_cast<double>(val));
                } else if constexpr (std::is_same_v<T, std::string>) {
                    _infoMessages.insert(QString::fromStdString(name), QString::fromStdString(val));
                }
            }, variant);
        } catch (const ulog_cpp::AccessException &e) {
            qCWarning(PX4LogLoaderLog) << "Failed to read info" << QString::fromStdString(name) << ":" << e.what();
        }
    }

    // --- Log messages ---
    for (const auto &entry : _dataContainer->logging()) {
        const QString line = QStringLiteral("[%1] %2: %3")
            .arg(entry.timestamp())
            .arg(QString::fromStdString(entry.logLevelStr()),
                 QString::fromStdString(entry.message()));
        _logMessages.append(line);
    }

    // --- Dropouts ---
    _dropoutCount = static_cast<int>(_dataContainer->dropouts().size());
}
