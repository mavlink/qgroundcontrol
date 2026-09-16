// clang-format off: Include order is the regression under test.
#ifdef QGC_GPS_CONTRACTS_FIRST
#include "GPSIOStatus.h"
#include "GPSTransportResult.h"
#include "GPSTransport.h"
#else
#include "GPSTransport.h"
#include "GPSTransportResult.h"
#include "GPSIOStatus.h"
#endif
// clang-format on

#include <type_traits>
#include <utility>

#include <QtCore/QVariant>

int main()
{
    static_assert(std::is_same_v<decltype(std::declval<GPSTransport&>().open()), GPSOpenResult>);
    static_assert(std::is_same_v<decltype(std::declval<GPSTransport&>().read(nullptr, 0, 0)), GPSReadResult>);
    static_assert(std::is_same_v<decltype(std::declval<GPSTransport&>().write(nullptr, 0)), GPSWriteResult>);
    static_assert(std::is_same_v<decltype(std::declval<GPSTransport&>().writeBounded(nullptr, 0, QDeadlineTimer())),
                                 GPSWriteResult>);
    static_assert(static_cast<int>(GPSOpenStatus::Unsupported) == 4);
    static_assert(static_cast<int>(GPSReadStatus::InvalidData) == 6);
    static_assert(static_cast<int>(GPSWriteStatus::InvalidData) == 5);

    const auto opened = QVariant::fromValue(GPSOpenResult{}).value<GPSOpenResult>();
    const auto read = QVariant::fromValue(GPSReadResult{}).value<GPSReadResult>();
    const auto emptyWrite = QVariant::fromValue(GPSWriteResult{}).value<GPSWriteResult>();
    const GPSWriteResult evidence{GPSWriteStatus::TimedOut, 12, 7, 5, QStringLiteral("deadline")};
    const auto written = QVariant::fromValue(evidence).value<GPSWriteResult>();
    return opened.status == GPSOpenStatus::Unsupported && opened.detail.isEmpty() &&
                   read.status == GPSReadStatus::TimedOut && read.bytesRead == 0 && read.detail.isEmpty() &&
                   emptyWrite.status == GPSWriteStatus::Unsupported && emptyWrite.acceptedBytes == 0 &&
                   emptyWrite.writtenBytes == 0 && emptyWrite.uncertainBytes == 0 && emptyWrite.detail.isEmpty() &&
                   written.status == evidence.status && written.acceptedBytes == 12 && written.writtenBytes == 7 &&
                   written.uncertainBytes == 5 && written.detail == evidence.detail
               ? 0
               : 1;
}
