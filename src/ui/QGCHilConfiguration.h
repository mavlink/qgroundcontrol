#ifndef QGCHILCONFIGURATION_H
#define QGCHILCONFIGURATION_H

#include <QWidget>

#include "QGCHilLink.h"
#include "UAS.h"

namespace Ui {
class QGCHilConfiguration;
}

class QGCHilConfiguration : public QWidget
{
    Q_OBJECT
    
public:
    QGCHilConfiguration(UAS* mav, QWidget *parent = 0);
    ~QGCHilConfiguration();

public slots:
    /** @brief Receive status message */
    void receiveStatusMessage(const QString& message);

protected:
    UAS* mav;
    
private slots:
    void on_simComboBox_currentIndexChanged(int index);

private:
    Ui::QGCHilConfiguration *ui;
};

#endif // QGCHILCONFIGURATION_H
