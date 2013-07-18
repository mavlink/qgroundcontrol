#ifndef AP2CONFIGWIDGET_H
#define AP2CONFIGWIDGET_H

#include <QWidget>
#include "UASManager.h"
#include "UASInterface.h"
class AP2ConfigWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AP2ConfigWidget(QWidget *parent = 0);
protected:
    UASInterface *m_uas;
    void showNullMAVErrorMessageBox();
signals:
    
public slots:
    virtual void activeUASSet(UASInterface *uas);
    virtual void parameterChanged(int uas, int component, QString parameterName, QVariant value);
};

#endif // AP2CONFIGWIDGET_H
