#ifndef COMPASSCONFIG_H
#define COMPASSCONFIG_H

#include <QWidget>
#include "ui_CompassConfig.h"
#include "UASManager.h"
#include "UASInterface.h"
class CompassConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit CompassConfig(QWidget *parent = 0);
    ~CompassConfig();
private slots:
    void activeUASSet(UASInterface *uas);
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void enableClicked(bool enabled);
    void autoDecClicked(bool enabled);
    void orientationComboChanged(int index);
private:
    Ui::CompassConfig ui;
    UASInterface *m_uas;
};

#endif // COMPASSCONFIG_H
