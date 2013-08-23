#ifndef APMPLANELEVEL_H
#define APMPLANELEVEL_H

#include <QWidget>
#include "ui_ApmPlaneLevel.h"
#include "AP2ConfigWidget.h"

class ApmPlaneLevel : public AP2ConfigWidget
{
    Q_OBJECT
    
public:
    explicit ApmPlaneLevel(QWidget *parent = 0);
    ~ApmPlaneLevel();
private slots:
    void parameterChanged(int uas, int component, QString parameterName, QVariant value);
    void levelClicked();
    void manualCheckBoxToggled(bool checked);
private:
    Ui::ApmPlaneLevel ui;
};

#endif // APMPLANELEVEL_H
