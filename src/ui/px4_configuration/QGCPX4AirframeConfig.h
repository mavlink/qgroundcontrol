#ifndef QGCPX4AIRFRAMECONFIG_H
#define QGCPX4AIRFRAMECONFIG_H

#include <QWidget>
#include <UASInterface.h>

namespace Ui {
class QGCPX4AirframeConfig;
}

class QGCPX4AirframeConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit QGCPX4AirframeConfig(QWidget *parent = 0);
    ~QGCPX4AirframeConfig();

public slots:

    /**
     * @brief Set the system currently operated on by this widget
     * @param uas The currently active / configured system
     */
    void setActiveUAS(UASInterface* uas);

    /**
     * @brief Handle parameter changes
     * @param uas
     * @param component
     * @param parameterName
     * @param value
     */
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);

    /**
     * @brief Quadrotor in X configuration has been selected
     */
    void quadXSelected();

    /**
     * @brief Quadrotor in X configuration has been selected with sub-type
     * @param index The autostart index which maps to a particular sub-type
     */
    void quadXSelected(int index);

    void flyingWingSelected();
    void flyingWingSelected(int index);
    void planeSelected();
    void planeSelected(int index);

    void quadPlusSelected();
    void quadPlusSelected(int index);
    void hexaXSelected();
    void hexaXSelected(int index);
    void hexaPlusSelected();
    void hexaPlusSelected(int index);
    void octoXSelected();
    void octoXSelected(int index);
    void octoPlusSelected();
    void octoPlusSelected(int index);
    void hSelected();
    void hSelected(int index);

    /**
     * @brief Apply changes and reboot system
     */
    void applyAndReboot();

protected:

    /**
     * @brief Set the ID of the current airframe
     * @param id the ID as defined by the PX4 SYS_AUTOSTART enum
     */
    void setAirframeID(int id);

    /**
     * @brief Enable automatic configuration
     * @param enabled If true, the system sets the default gains for this platform on the next boot
     */
    void setAutoConfig(bool enabled);

    
private:
    UASInterface* mav;
    int selectedId;
    Ui::QGCPX4AirframeConfig *ui;
};

#endif // QGCPX4AIRFRAMECONFIG_H
