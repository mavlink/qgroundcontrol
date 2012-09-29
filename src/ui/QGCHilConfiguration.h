#ifndef QGCHILCONFIGURATION_H
#define QGCHILCONFIGURATION_H

#include <QWidget>

#include "QGCHilLink.h"

namespace Ui {
class QGCHilConfiguration;
}

class QGCHilConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    QGCHilConfiguration(QGCHilLink* link, QWidget *parent = 0);
    ~QGCHilConfiguration();

public slots:
    /** @brief Start / stop simulation */
    void toggleSimulation(bool connect);
    /** @brief Receive status message */
    void receiveStatusMessage(const QString& message);

protected:
    QGCHilLink* link;
    
private:
    Ui::QGCHilConfiguration *ui;
};

#endif // QGCHILCONFIGURATION_H
