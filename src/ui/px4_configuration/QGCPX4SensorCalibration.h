#ifndef QGCPX4SENSORCALIBRATION_H
#define QGCPX4SENSORCALIBRATION_H

#include <QWidget>
#include <UASInterface.h>
#include <QAction>

namespace Ui {
class QGCPX4SensorCalibration;
}

class QGCPX4SensorCalibration : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCPX4SensorCalibration(QWidget *parent = 0);
    ~QGCPX4SensorCalibration();

public slots:
    /**
     * @brief Set currently active UAS
     * @param uas the current active UAS
     */
    void setActiveUAS(UASInterface* uas);

    /**
     * @brief Handle text message from current active UAS
     * @param uasid
     * @param componentid
     * @param severity
     * @param text
     */
    void handleTextMessage(int uasid, int componentid, int severity, QString text);

    /**
     * @brief Update system specs / properties
     * @param id the UID of the aircraft
     */
    void updateSystemSpecs(int id);

    void gyroButtonClicked();
    void magButtonClicked();
    void accelButtonClicked();
    void diffPressureButtonClicked();

    /**
     * @brief Hand context menu event
     * @param event
     */
    virtual void contextMenuEvent(QContextMenuEvent* event);

    void setAutopilotOrientation(int index);
    void setGpsOrientation(int index);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);

protected slots:

    void setInstructionImage(const QString &path);

    void setAutopilotImage(const QString &path);

    void setGpsImage(const int index);

    void setAutopilotImage(const int index);

    void setGpsImage(const QString &path);

protected:
    UASInterface* activeUAS;
    QAction* clearAction;
    QPixmap instructionIcon;
    QPixmap autopilotIcon;
    QPixmap gpsIcon;

    virtual void resizeEvent(QResizeEvent* event);

    void setMagCalibrated(bool calibrated);
    void setGyroCalibrated(bool calibrated);
    void setAccelCalibrated(bool calibrated);
    void setDiffPressureCalibrated(bool calibrated);

    void updateIcons();
    
private:
    void _requestAllSensorParameters(void);
    
    Ui::QGCPX4SensorCalibration *ui;

};

#endif // QGCPX4SENSORCALIBRATION_H
