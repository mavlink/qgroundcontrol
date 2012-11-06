#ifndef PX4FIRMWAREUPGRADEWORKER_H
#define PX4FIRMWAREUPGRADEWORKER_H

#include <QObject>

#include <SerialLink.h>

class PX4FirmwareUpgradeWorker : public QObject
{
    Q_OBJECT
public:
    explicit PX4FirmwareUpgradeWorker(QObject *parent = 0);
    static PX4FirmwareUpgradeWorker* putWorkerInThread(QObject *parent);

signals:
    void detectionStatusChanged(const QString& status);
    void upgradeStatusChanged(const QString& status);
    void upgradeProgressChanged(int percent);
    void validPortFound(const QString& portName);
    
public slots:

    /**
     * @brief Continously scan for bootloaders
     * @return
     */
    bool startContinousScan();

    /**
     * @brief Detect connected PX4 bootloaders
     *
     * If a bootloader was found, the link will be opened to this
     * bootloader and ready for flashing when returning from the call.
     *
     * @return true if found on one link, false else
     */
    bool detect();

    /**
     * @brief Upgrade the firmware using the currently connected link
     * @param filename file name / path of the firmware file
     * @return true if upgrade succeeds, false else
     */
    bool upgrade(const QString &filename);

    /**
     * @brief Receive bytes from a link
     * @param link
     * @param b
     */
    void receiveBytes(LinkInterface* link, QByteArray b);

private:
    SerialLink *link;
    bool insync;
};

#endif // PX4FIRMWAREUPGRADEWORKER_H
