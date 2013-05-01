#ifndef QGCTEXTLABEL_H
#define QGCTEXTLABEL_H

#include "QGCToolWidgetItem.h"

namespace Ui
{
class QGCTextLabel;
}

class UASInterface;

class QGCTextLabel : public QGCToolWidgetItem
{
    Q_OBJECT

public:
    explicit QGCTextLabel(QWidget *parent = 0);
    ~QGCTextLabel();
    void setActiveUAS(UASInterface *uas);
    void enableText(int num);
public slots:
    void startEditMode();
    void endEditMode();
    void writeSettings(QSettings& settings);
    void readSettings(const QSettings& settings);
    void readSettings(const QString& pre,const QVariantMap& settings);
    void textMessageReceived(int uasid, int component, int severity, QString message);
private:
    int enabledNum;
    Ui::QGCTextLabel *ui;
};

#endif // QGCTEXTLABEL_H
