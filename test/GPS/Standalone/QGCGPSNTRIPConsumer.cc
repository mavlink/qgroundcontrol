#include <QtCore/QCoreApplication>

#include "NTRIPConnectionStats.h"
#include "NTRIPGgaProvider.h"
#include "NTRIPHttpTransport.h"
#include "NTRIPSourceTable.h"
#include "NTRIPSourceTableController.h"

#if defined(QT_QML_LIB) || defined(QT_GUI_LIB) || defined(QT_SERIALPORT_LIB) || defined(QT_BLUETOOTH_LIB) || \
    defined(QT_HTTPSERVER_LIB)
#error NTRIP must not inherit application, serial, Bluetooth, or HTTP server dependencies.
#endif

class RequestConsumer : public NTRIPHttpTransport
{
public:
    using NTRIPHttpTransport::buildHttpRequest;
};

class GgaConsumer : public NTRIPTransport
{
public:
    void start() override {}

    void stop() override {}

    void sendNMEA(const QByteArray& nmea) override { sentence = nmea; }

    QByteArray sentence;
};

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    NTRIPConnectionConfig config;
    config.host = QStringLiteral("localhost");
    config.mountpoint = QStringLiteral("BASE");
    config.username = QStringLiteral("user");
    config.password = QStringLiteral("pass");
    const auto request = RequestConsumer::buildHttpRequest(config);
    if (!request.error.isEmpty() || !request.credentialsInClear ||
        !request.bytes.contains("Authorization: Basic dXNlcjpwYXNz\r\n")) {
        return 1;
    }
    NTRIPHttpTransport http(config, {});
    NTRIPSourceTableController table;
    if (table.fetchStatus() != NTRIPSourceTableController::FetchStatus::Idle || !table.mountpointModel()) {
        return 2;
    }
    NTRIPConnectionStats stats;
    stats.recordMessage(42, 1005);
    if (stats.bytesReceived() != 42 || stats.messagesReceived() != 1) {
        return 3;
    }
    GgaConsumer transport;
    NTRIPGgaProvider provider;
    provider.setPositionProvider(NTRIPGgaProvider::PositionSource::GCSPosition, []() {
        return PositionResult{QGeoCoordinate(47, 8, 500), QStringLiteral("Consumer")};
    });
    provider.start(&transport);
    provider.stop();
    return transport.sentence.startsWith("$GPGGA,") ? 0 : 4;
}
