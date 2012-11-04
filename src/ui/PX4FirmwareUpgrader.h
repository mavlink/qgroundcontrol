#ifndef PX4FIRMWAREUPGRADER_H
#define PX4FIRMWAREUPGRADER_H

#include <QWidget>
#include <QTimer>

#include <SerialLink.h>

namespace Ui {
class PX4FirmwareUpgrader;
}

class PX4FirmwareUpgrader : public QWidget
{
    Q_OBJECT
    
public:
    PX4FirmwareUpgrader(QWidget *parent = 0);
    ~PX4FirmwareUpgrader();

public slots:

    void setDetectionStatusText(const QString &text);

    void setFlashStatusText(const QString &text);

    void setFlashProgress(int percent);

    void selectFirmwareFile();

    void setPortName(const QString &portname);

signals:
    void firmwareFileNameSet(const QString &fileName);
    void upgrade();

private:
    Ui::PX4FirmwareUpgrader *ui;
};

#endif // PX4FIRMWAREUPGRADER_H
