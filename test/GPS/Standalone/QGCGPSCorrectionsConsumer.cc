#include <QtCore/QCoreApplication>

#include "GPSCorrectionEventModel.h"
#include "GPSCorrectionRouter.h"

#if defined(QT_POSITIONING_LIB) || defined(QT_NETWORK_LIB) || defined(QT_QML_LIB)
#error Corrections must not inherit positioning, transport, or application dependencies.
#endif

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    constexpr qint64 now = 1000;
    GPSCorrectionRouter router(nullptr, [] { return now; });
    auto source = router.registerSource(GPSCorrectionSource::Ntrip, QStringLiteral("caster"));
    GPSCorrectionFrame admitted;
    router.setOutput(
        QStringLiteral("consumer"),
        {.scope = GPSCorrectionSource::Unknown,
         .completion = GPSCorrectionRouter::Completion::AdmissionOnly,
         .admit = [&](const GPSCorrectionFrame& frame) {
             admitted = frame;
             return QList<GPSCorrectionRouter::Admission>{
                 {QStringLiteral("consumer"), {quint64(frame.data.size()), 0, GPSCorrectionReason::None}, true}};
         }});
    const auto bytes = QByteArray::fromHex("d300023ed0a4e000");
    if (!source.token().valid() || !router.acceptIngress(source.token().event(bytes, now, 1005, true)) ||
        admitted.data != bytes || router.activeSource() != GPSCorrectionSource::Ntrip) {
        return 1;
    }
    const auto& stats = router.statistics().at(static_cast<int>(GPSCorrectionSource::Ntrip));
    if (stats.queuedBytes != quint64(bytes.size()) || stats.writtenBytes != 0 || stats.transportAcceptedBytes != 0) {
        return 2;
    }
    GPSCorrectionEventModel events;
    events.setEvents(router.events());
    if (events.rowCount() != 4 || events.index(3).data(GPSCorrectionEventModel::StageRole).toInt() !=
                                      static_cast<int>(GPSCorrectionStage::Queued)) {
        return 3;
    }
    const auto retired = source.token();
    source.reset();
    return retired.valid() || router.acceptIngress(retired.event(bytes, now, 1005, true)) ? 4 : 0;
}
