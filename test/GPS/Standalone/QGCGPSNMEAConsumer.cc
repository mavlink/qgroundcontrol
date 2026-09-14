#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>

#include "NMEADecoderSession.h"
#include "NMEAUtils.h"

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    QBuffer input;
    input.open(QIODevice::ReadOnly);
    NMEADecoderSession session;
    if (!session.start(&input) || !session.positionSource() || session.health()->usable()) {
        return 1;
    }
    const auto sentence = NMEAUtils::repairChecksum("$GPGSV,1,1,01,01,40,083,41");
    if (!NMEAUtils::verifyChecksum(sentence)) {
        return 2;
    }
    session.stop();
    return input.isOpen() && session.satelliteObservation().satellitesInViewCount() == -1 ? 0 : 3;
}
